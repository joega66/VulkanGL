#include "CameraProxy.h"
#include <Engine/Engine.h>
#include <Engine/Screen.h>
#include <Components/Bounds.h>

UNIFORM_STRUCT(CameraUniformBufferParams,
	glm::mat4 worldToView;
	glm::mat4 viewToClip;
	glm::mat4 worldToClip;
	glm::mat4 clipToWorld;
	glm::vec3 position;
	float _pad0;
	float aspectRatio;
	float fov;
	glm::vec2 screenDims;
	glm::vec3 clipData;
	float _pad1;
);

CameraProxy::CameraProxy(Engine& engine)
	: _CameraDescriptorSet(engine.Device)
{
	_CameraUniformBuffer = engine.Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(CameraUniformBufferParams));
	_CameraDescriptorSet._CameraUniform = _CameraUniformBuffer;

	engine._Screen.OnScreenResize([this, &engine] (int32 width, int32 height)
	{
		auto& device = engine.Device;
		auto& surface = engine.Surface;

		_SceneColor = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Attachment | EImageUsage::Storage | EImageUsage::TransferDst);
		_SceneDepth = device.CreateImage(width, height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

		_GBuffer0 = device.CreateImage(width, height, 1, EFormat::R32G32B32A32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
		_GBuffer1 = device.CreateImage(width, height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Sampled);
		
		_SSGIHistory = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Storage);

		CreateSceneRP(device);
		CreateGBufferRP(device);
		CreateUserInterfaceRP(device, surface);

		const gpu::Sampler sampler = device.CreateSampler({ EFilter::Nearest });

		_CameraDescriptorSet._SceneDepth = gpu::DescriptorImageInfo(_SceneDepth, sampler);
		_CameraDescriptorSet._GBuffer0 = gpu::DescriptorImageInfo(_GBuffer0, sampler);
		_CameraDescriptorSet._GBuffer1 = gpu::DescriptorImageInfo(_GBuffer1, sampler);
		_CameraDescriptorSet._SceneColor = gpu::DescriptorImageInfo(_SceneColor);
		_CameraDescriptorSet._SSGIHistory = gpu::DescriptorImageInfo(_SSGIHistory);
		_CameraDescriptorSet.Update(device);

		gpu::CommandList cmdList = device.CreateCommandList(EQueue::Transfer);

		const ImageMemoryBarrier imageBarrier
		{
			_SSGIHistory,
			EAccess::None,
			EAccess::ShaderRead | EAccess::ShaderWrite,
			EImageLayout::Undefined,
			EImageLayout::General
		};

		cmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);

		device.SubmitCommands(cmdList);
	});
}

void CameraProxy::Update(Engine& engine)
{
	UpdateCameraUniform(engine);
	BuildMeshDrawCommands(engine);
}

void CameraProxy::UpdateCameraUniform(Engine& engine)
{
	const Camera& camera = engine.Camera;

	const glm::vec3 clipData(
		camera.GetFarPlane() * camera.GetNearPlane(),
		camera.GetNearPlane() - camera.GetFarPlane(),
		camera.GetFarPlane());

	const CameraUniformBufferParams cameraUniformBufferParams =
	{
		camera.GetWorldToView(),
		camera.GetViewToClip(),
		camera.GetWorldToClip(),
		glm::inverse(camera.GetWorldToClip()),
		camera.GetPosition(),
		0.0f,
		camera.GetAspectRatio(),
		camera.GetFOV(),
		glm::vec2(camera.GetWidth(), camera.GetHeight()),
		clipData,
		0.0f,
	};

	Platform::Memcpy(_CameraUniformBuffer.GetData(), &cameraUniformBufferParams, sizeof(cameraUniformBufferParams));
}

void CameraProxy::BuildMeshDrawCommands(Engine& engine)
{
	_GBufferPass.clear();

	const FrustumPlanes viewFrustumPlanes = engine.Camera.GetFrustumPlanes();
	
	for (auto entity : engine._ECS.GetEntities<MeshProxy>())
	{
		const auto& meshProxy = engine._ECS.GetComponent<MeshProxy>(entity);
		const auto& bounds = engine._ECS.GetComponent<Bounds>(entity);

		if (Physics::IsBoxInsideFrustum(viewFrustumPlanes, bounds.Box))
		{
			AddToGBufferPass(engine, meshProxy);
		}
	}
}

void CameraProxy::CreateSceneRP(gpu::Device& device)
{
	RenderPassDesc rpDesc = {};
	rpDesc.colorAttachments.push_back(
		gpu::AttachmentView(&_SceneColor, ELoadAction::Load, EStoreAction::Store, ClearColorValue{}, EImageLayout::General, EImageLayout::General));
	rpDesc.depthAttachment = gpu::AttachmentView(
		&_SceneDepth,
		ELoadAction::Load,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::DepthReadStencilWrite,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2(), glm::uvec2(_SceneDepth.GetWidth(), _SceneDepth.GetHeight()) };
	_SceneRP = device.CreateRenderPass(rpDesc);
}

void CameraProxy::CreateGBufferRP(gpu::Device& device)
{
	RenderPassDesc rpDesc = {};
	rpDesc.colorAttachments =
	{
		gpu::AttachmentView(&_GBuffer0, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal),
		gpu::AttachmentView(&_GBuffer1, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal)
	};
	rpDesc.depthAttachment = gpu::AttachmentView(
		&_SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2(), glm::uvec2(_SceneDepth.GetWidth(), _SceneDepth.GetHeight()) };
	_GBufferRP = device.CreateRenderPass(rpDesc);
}

void CameraProxy::CreateUserInterfaceRP(gpu::Device& device, gpu::Surface& surface)
{
	const auto& images = surface.GetImages();

	_UserInterfaceRP.clear();
	_UserInterfaceRP.reserve(images.size());

	for (const auto& image : images)
	{
		// After UI rendering, the image is ready for the present queue.
		RenderPassDesc rpDesc = {};
		rpDesc.colorAttachments.push_back(
			gpu::AttachmentView(&image, ELoadAction::Load, EStoreAction::Store, ClearColorValue{}, EImageLayout::ColorAttachmentOptimal, EImageLayout::Present));
		rpDesc.renderArea.extent = { image.GetWidth(), image.GetHeight() };

		_UserInterfaceRP.push_back(device.CreateRenderPass(rpDesc));
	}
}
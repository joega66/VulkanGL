#include "CameraProxy.h"
#include <Engine/Engine.h>
#include <Engine/Screen.h>
#include <Components/Bounds.h>

UNIFORM_STRUCT(CameraUniformBufferShaderParameters,
	glm::mat4 WorldToView;
	glm::mat4 ViewToClip;
	glm::mat4 WorldToClip;
	glm::mat4 ClipToWorld;
	glm::vec3 Position;
	float _Pad0;
	float AspectRatio;
	float FOV;
	glm::vec2 ScreenDims;
);

CameraProxy::CameraProxy(Engine& Engine)
	: CameraDescriptorSet(Engine.Device)
{
	CameraUniformBuffer = Engine.Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(CameraUniformBufferShaderParameters));
	CameraDescriptorSet.CameraUniform = CameraUniformBuffer;

	Engine._Screen.OnScreenResize([this, &Engine] (int32 Width, int32 Height)
	{
		auto& Device = Engine.Device;
		auto& Surface = Engine.Surface;

		SceneColor = Device.CreateImage(Width, Height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Attachment | EImageUsage::Storage | EImageUsage::TransferDst);
		SceneDepth = Device.CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

		GBuffer0 = Device.CreateImage(Width, Height, 1, EFormat::R32G32B32A32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
		GBuffer1 = Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Sampled);

		CreateSceneRP(Device);
		CreateGBufferRP(Device);
		CreateUserInterfaceRP(Device, Surface);

		const gpu::Sampler Sampler = Device.CreateSampler({ EFilter::Nearest });

		CameraDescriptorSet.SceneDepth = gpu::DescriptorImageInfo(SceneDepth, Sampler);
		CameraDescriptorSet.GBuffer0 = gpu::DescriptorImageInfo(GBuffer0, Sampler);
		CameraDescriptorSet.GBuffer1 = gpu::DescriptorImageInfo(GBuffer1, Sampler);
		CameraDescriptorSet.SceneColor = gpu::DescriptorImageInfo(SceneColor);
		CameraDescriptorSet.Update(Device);
	});
}

void CameraProxy::Update(Engine& Engine)
{
	UpdateCameraUniform(Engine);
	BuildMeshDrawCommands(Engine);
}

void CameraProxy::UpdateCameraUniform(Engine& Engine)
{
	const Camera& Camera = Engine.Camera;

	const CameraUniformBufferShaderParameters CameraUniformBufferShaderParameters =
	{
		Camera.GetWorldToView(),
		Camera.GetViewToClip(),
		Camera.GetWorldToClip(),
		glm::inverse(Camera.GetWorldToClip()),
		Camera.GetPosition(),
		0.0f,
		Camera.GetAspectRatio(),
		Camera.GetFOV(),
		glm::vec2(Camera.GetWidth(), Camera.GetHeight())
	};

	Platform::Memcpy(CameraUniformBuffer.GetData(), &CameraUniformBufferShaderParameters, sizeof(CameraUniformBufferShaderParameters));
}

void CameraProxy::BuildMeshDrawCommands(Engine& Engine)
{
	GBufferPass.clear();

	const FrustumPlanes ViewFrustumPlanes = Engine.Camera.GetFrustumPlanes();
	
	for (auto Entity : Engine.ECS.GetEntities<MeshProxy>())
	{
		const auto& MeshProxy = Engine.ECS.GetComponent<class MeshProxy>(Entity);
		const auto& Bounds = Engine.ECS.GetComponent<class Bounds>(Entity);

		if (Physics::IsBoxInsideFrustum(ViewFrustumPlanes, Bounds.Box))
		{
			AddToGBufferPass(Engine, MeshProxy);
		}
	}
}

void CameraProxy::CreateSceneRP(gpu::Device& Device)
{
	RenderPassDesc rpDesc = {};
	rpDesc.colorAttachments.push_back(
		gpu::AttachmentView(&SceneColor, ELoadAction::Load, EStoreAction::Store, ClearColorValue{}, EImageLayout::General, EImageLayout::General)
	);
	rpDesc.depthAttachment = gpu::AttachmentView(
		&SceneDepth,
		ELoadAction::Load,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::DepthReadStencilWrite,
		EImageLayout::DepthReadStencilWrite
	);
	rpDesc.renderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	SceneRP = Device.CreateRenderPass(rpDesc);
}

void CameraProxy::CreateGBufferRP(gpu::Device& Device)
{
	RenderPassDesc rpDesc = {};
	rpDesc.colorAttachments =
	{
		gpu::AttachmentView(&GBuffer0, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal),
		gpu::AttachmentView(&GBuffer1, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal)
	};
	rpDesc.depthAttachment = gpu::AttachmentView(
		&SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	GBufferRP = Device.CreateRenderPass(rpDesc);
}

void CameraProxy::CreateUserInterfaceRP(gpu::Device& Device, gpu::Surface& Surface)
{
	const auto& Images = Surface.GetImages();

	UserInterfaceRP.clear();
	UserInterfaceRP.reserve(Images.size());

	for (const auto& Image : Images)
	{
		// After UI rendering, the image is ready for the present queue.
		RenderPassDesc rpDesc = {};
		rpDesc.colorAttachments.push_back(
			gpu::AttachmentView(&Image, ELoadAction::Load, EStoreAction::Store, ClearColorValue{}, EImageLayout::ColorAttachmentOptimal, EImageLayout::Present)
		);
		rpDesc.renderArea.extent = { Image.GetWidth(), Image.GetHeight() };

		UserInterfaceRP.push_back(Device.CreateRenderPass(rpDesc));
	}
}
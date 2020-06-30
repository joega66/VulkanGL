#include "CameraProxy.h"
#include <Engine/Engine.h>
#include <Engine/Screen.h>
#include <Components/Bounds.h>

UNIFORM_STRUCT(CameraUniformBufferShaderParameters,
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
	const Camera& camera = Engine.Camera;

	const glm::vec3 clipData(
		camera.GetFarPlane() * camera.GetNearPlane(),
		camera.GetNearPlane() - camera.GetFarPlane(),
		camera.GetFarPlane());

	const CameraUniformBufferShaderParameters CameraUniformBufferShaderParameters =
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
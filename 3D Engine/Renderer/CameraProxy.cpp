#include "CameraProxy.h"
#include "GlobalRenderData.h"
#include <Engine/Engine.h>
#include <Engine/Screen.h>

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

	Engine._Screen.ScreenResizeEvent([this, &Engine] (int32 Width, int32 Height)
	{
		SceneColor = Engine.Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Storage | EImageUsage::TransferSrc | EImageUsage::TransferDst);
		SceneDepth = Engine.Device.CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

		GBuffer0 = Engine.Device.CreateImage(Width, Height, 1, EFormat::R32G32B32A32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
		GBuffer1 = Engine.Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Sampled);

		CreateSceneRP(Engine.Device);
		CreateGBufferRP(Engine.Device);

		auto& GlobalData = Engine.ECS.GetSingletonComponent<GlobalRenderData>();
		GlobalData.VCTLightingCache.CreateDebugRenderPass(SceneColor, SceneDepth);

		const drm::Sampler Sampler = Engine.Device.CreateSampler({ EFilter::Nearest });

		CameraDescriptorSet.SceneDepth = drm::DescriptorImageInfo(SceneDepth, Sampler);
		CameraDescriptorSet.GBuffer0 = drm::DescriptorImageInfo(GBuffer0, Sampler);
		CameraDescriptorSet.GBuffer1 = drm::DescriptorImageInfo(GBuffer1, Sampler);
		CameraDescriptorSet.RadianceVolume = drm::DescriptorImageInfo(GlobalData.VCTLightingCache.GetVoxelRadiance(), GlobalData.VCTLightingCache.GetVoxelRadianceSampler());
		CameraDescriptorSet.SceneColor = drm::DescriptorImageInfo(SceneColor);
		CameraDescriptorSet.Update(Engine.Device);
	});

	GlobalRenderData& GlobalData = Engine.ECS.GetSingletonComponent<GlobalRenderData>();

	GlobalData.SkyboxDescriptorSet.Skybox = drm::DescriptorImageInfo(*Engine.Scene.Skybox, Engine.Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }));
	GlobalData.SkyboxDescriptorSet.Update(Engine.Device);
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
	VoxelsPass.clear();

	const FrustumPlanes ViewFrustumPlanes = Engine.Camera.GetFrustumPlanes();
	
	for (auto Entity : Engine.ECS.GetEntities<MeshProxy>())
	{
		const MeshProxy& MeshProxy = Engine.ECS.GetComponent<class MeshProxy>(Entity);

		if (Physics::IsBoxInsideFrustum(ViewFrustumPlanes, MeshProxy.WorldSpaceBB))
		{
			AddToGBufferPass(Engine, MeshProxy);
		}

		AddToVoxelsPass(Engine, MeshProxy);
	}
}

void CameraProxy::CreateSceneRP(DRMDevice& Device)
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments.push_back(
		drm::AttachmentView(&SceneColor, ELoadAction::Load, EStoreAction::Store, ClearColorValue{}, EImageLayout::ColorAttachmentOptimal, EImageLayout::TransferSrcOptimal)
	);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	SceneRP = Device.CreateRenderPass(RPDesc);
}

void CameraProxy::CreateGBufferRP(DRMDevice& Device)
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments =
	{
		drm::AttachmentView(&GBuffer0, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal),
		drm::AttachmentView(&GBuffer1, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal)
	};
	RPDesc.DepthAttachment = drm::AttachmentView(
		&SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	GBufferRP = Device.CreateRenderPass(RPDesc);
}
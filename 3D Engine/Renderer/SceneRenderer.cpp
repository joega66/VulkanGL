#include "SceneRenderer.h"
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include <DRMShader.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>

SceneRenderer::SceneRenderer(DRM& Device, Scene& Scene, Screen& Screen)
	: Device(Device)
{
	SceneTextures = Device.CreateDescriptorSet();

	Screen.ScreenResizeEvent([&](int32 Width, int32 Height)
	{
		SceneDepth = Device.CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
		ShadowMask = Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Sampled);

		SceneTextures->Write(SceneDepth, SamplerState{ EFilter::Nearest }, 0);
		SceneTextures->Write(ShadowMask, SamplerState{ EFilter::Nearest }, 1);
		SceneTextures->Update();
	});

	Cube = Scene.Assets.GetStaticMesh("Cube");

	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);
	VoxelColors = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage);
	VoxelPositions = Device.CreateBuffer(EBufferUsage::Storage, VoxelGridSize * VoxelGridSize * VoxelGridSize * sizeof(glm::ivec3));

	VoxelsDescriptorSet = Device.CreateDescriptorSet();
	VoxelsDescriptorSet->Write(VoxelColors, 1);
	VoxelsDescriptorSet->Write(VoxelPositions, 2);

	ShadowProxy::InitCallbacks(Device, Scene.ECS);
}

void SceneRenderer::Render(SceneProxy& Scene)
{
	Device.BeginFrame();

	drm::CommandListRef CommandList = Device.CreateCommandList();
	drm::CommandList& CmdList = *CommandList;

	if (Platform::GetBool("Engine.ini", "Voxels", "RenderVoxels", false))
	{
		RenderVoxels(Scene, CmdList);
	}
	else
	{
		RenderDepthPrepass(Scene, CmdList);

		RenderShadowDepths(Scene, CmdList);

		if (Platform::GetBool("Engine.ini", "Shadows", "Visualize", false))
		{
			RenderDepthVisualization(Scene, CmdList);
		}
		else
		{
			drm::AttachmentView SurfaceView(Device.GetSurface(), ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Present);
			drm::AttachmentView DepthView(SceneDepth, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{}, EImageLayout::DepthReadStencilWrite);

			RenderPassInitializer RenderPassInit = { 1 };
			RenderPassInit.ColorAttachments[0] = SurfaceView;
			RenderPassInit.DepthAttachment = DepthView;
			RenderPassInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

			CmdList.BeginRenderPass(RenderPassInit);

			RenderLightingPass(Scene, CmdList);

			RenderSkybox(Scene, CmdList);

			CmdList.EndRenderPass();
		}
	}

	Device.SubmitCommands(CommandList);

	Device.EndFrame();
}

void SceneRenderer::RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	drm::AttachmentView SurfaceView(Device.GetSurface(), ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Present);

	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		auto& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);
		auto ShadowMap = ShadowProxy.GetShadowMap();

		RenderPassInitializer RPInit = { 1 };
		RPInit.ColorAttachments[0] = SurfaceView;
		RPInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

		CmdList.BeginRenderPass(RPInit);

		drm::DescriptorSetRef DescriptorSet = Device.CreateDescriptorSet();
		DescriptorSet->Write(ShadowMap, SamplerState{ EFilter::Nearest }, 0);
		DescriptorSet->Update();

		CmdList.BindDescriptorSets(1, &DescriptorSet);

		PipelineStateInitializer PSOInit = {};
		PSOInit.Viewport.Width = SceneDepth->Width;
		PSOInit.Viewport.Height = SceneDepth->Height;
		PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSOInit.DepthStencilState.DepthWriteEnable = false;
		PSOInit.ShaderStages.Vertex = Scene.ShaderMap.FindShader<FullscreenVS>();
		PSOInit.ShaderStages.Fragment = Scene.ShaderMap.FindShader<FullscreenFS<EVisualize::Depth>>();

		CmdList.BindPipeline(PSOInit);

		CmdList.Draw(3, 1, 0, 0);

		CmdList.EndRenderPass();
	}
}
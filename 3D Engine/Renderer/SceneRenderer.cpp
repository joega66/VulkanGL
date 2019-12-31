#include "SceneRenderer.h"
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>

uint32 gVoxelGridSize;

SceneRenderer::SceneRenderer(Scene& Scene)
{
	SceneTextures = drm::CreateDescriptorSet();

	gScreen.RegisterScreenResChangedCallback([&](int32 Width, int32 Height)
	{
		SceneDepth = drm::CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
		ShadowMask = drm::CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Sampled);

		SceneTextures->Write(SceneDepth, SamplerState{ EFilter::Nearest }, 0);
		SceneTextures->Write(ShadowMask, SamplerState{ EFilter::Nearest }, 1);
		SceneTextures->Update();
	});

	Cube = Scene.Assets.GetStaticMesh("Cube");

	gVoxelGridSize = Platform.GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);
	VoxelColors = drm::CreateImage(gVoxelGridSize, gVoxelGridSize, gVoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage);
	VoxelPositions = drm::CreateBuffer(EBufferUsage::Storage, gVoxelGridSize * gVoxelGridSize * gVoxelGridSize * sizeof(glm::ivec3));

	VoxelsDescriptorSet = drm::CreateDescriptorSet();
	VoxelsDescriptorSet->Write(VoxelColors, 1);
	VoxelsDescriptorSet->Write(VoxelPositions, 2);

	ShadowProxy::InitCallbacks(Scene.ECS);
}

void SceneRenderer::Render(SceneProxy& Scene)
{
	drm::BeginFrame();

	drm::CommandListRef CommandList = drm::CreateCommandList();
	drm::CommandList& CmdList = *CommandList;

	if (Platform.GetBool("Engine.ini", "Voxels", "RenderVoxels", false))
	{
		RenderVoxels(Scene, CmdList);
	}
	else
	{
		RenderDepthPrepass(Scene, CmdList);

		RenderShadowDepths(Scene, CmdList);

		if (Platform.GetBool("Engine.ini", "Shadows", "Visualize", false))
		{
			RenderDepthVisualization(Scene, CmdList);
		}
		else
		{
			drm::AttachmentView SurfaceView(drm::GetSurface(), ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Present);
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

	drm::SubmitCommands(CommandList);

	drm::EndFrame();
}

void SceneRenderer::RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	drm::AttachmentView SurfaceView(drm::GetSurface(), ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Present);

	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		auto& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);
		auto ShadowMap = ShadowProxy.GetShadowMap();

		RenderPassInitializer RPInit = { 1 };
		RPInit.ColorAttachments[0] = SurfaceView;
		RPInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

		CmdList.BeginRenderPass(RPInit);

		drm::DescriptorSetRef DescriptorSet = drm::CreateDescriptorSet();
		DescriptorSet->Write(ShadowMap, SamplerState{ EFilter::Nearest }, 0);
		DescriptorSet->Update();

		CmdList.BindDescriptorSets(1, &DescriptorSet);

		PipelineStateInitializer PSOInit = {};
		PSOInit.Viewport.Width = SceneDepth->Width;
		PSOInit.Viewport.Height = SceneDepth->Height;
		PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSOInit.DepthStencilState.DepthWriteEnable = false;
		PSOInit.ShaderStages.Vertex = *ShaderMapRef<FullscreenVS>();
		PSOInit.ShaderStages.Fragment = *ShaderMapRef<FullscreenFS<EVisualize::Depth>>();

		CmdList.BindPipeline(PSOInit);

		CmdList.Draw(3, 1, 0, 0);

		CmdList.EndRenderPass();
	}
}
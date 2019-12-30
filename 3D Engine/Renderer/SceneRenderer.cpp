#include "SceneRenderer.h"
#include "FullscreenQuad.h"
#include "RayMarching.h"
#include "Light.h"
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>

SceneRenderer::SceneRenderer(const Scene& Scene)
{
	RenderTargetsSet = drm::CreateDescriptorSet();

	gScreen.RegisterScreenResChangedCallback([&](int32 Width, int32 Height)
	{
		SceneDepth = drm::CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::RenderTargetable | EImageUsage::Sampled);
		ShadowMask = drm::CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::RenderTargetable | EImageUsage::Sampled);
	});

	Cube = Scene.Assets.GetStaticMesh("Cube");

	gVoxelGridSize = Platform.GetInt("Engine.ini", "Renderer", "VoxelGridSize", 256);
	VoxelColors = drm::CreateImage(gVoxelGridSize, gVoxelGridSize, gVoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage);
	VoxelPositions = drm::CreateBuffer(EBufferUsage::Storage, gVoxelGridSize * gVoxelGridSize * gVoxelGridSize * sizeof(glm::ivec3));

	VoxelsDescriptorSet = drm::CreateDescriptorSet();
	VoxelsDescriptorSet->Write(VoxelColors, ShaderBinding(1));
	VoxelsDescriptorSet->Write(VoxelPositions, ShaderBinding(2));
}

void SceneRenderer::Render(SceneProxy& Scene)
{
	drm::BeginFrame();

	RenderCommandListRef CommandList = drm::CreateCommandList();
	RenderCommandList& CmdList = *CommandList;

	if (Platform.GetBool("Engine.ini", "Renderer", "RenderVoxels", false))
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
			drm::RenderTargetView SurfaceView(drm::GetSurface(), ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Present);
			drm::RenderTargetView DepthView(SceneDepth, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{}, EImageLayout::DepthReadStencilWrite);

			RenderPassInitializer RenderPassInit = { 1 };
			RenderPassInit.ColorTargets[0] = SurfaceView;
			RenderPassInit.DepthTarget = DepthView;
			RenderPassInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

			CmdList.BeginRenderPass(RenderPassInit);

			RenderLightingPass(Scene, CmdList);

			RenderSkybox(Scene, CmdList);

			CmdList.EndRenderPass();
		}
	}

	CmdList.Finish();

	drm::SubmitCommands(CommandList);

	drm::EndFrame();
}

void SceneRenderer::RenderDepthVisualization(SceneProxy& Scene, RenderCommandList& CmdList)
{
	drm::RenderTargetView SurfaceView(drm::GetSurface(), ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Present);

	for (auto Entity : Scene.ECS.GetVisibleEntities<CShadowProxy>())
	{
		auto& ShadowProxy = Scene.ECS.GetComponent<CShadowProxy>(Entity);
		auto ShadowMap = ShadowProxy.GetShadowMap();

		RenderPassInitializer RPInit = { 1 };
		RPInit.ColorTargets[0] = SurfaceView;
		RPInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

		CmdList.BeginRenderPass(RPInit);

		drm::DescriptorSetRef DescriptorSet = drm::CreateDescriptorSet();
		DescriptorSet->Write(ShadowMap, SamplerState{ EFilter::Nearest }, ShaderBinding(0));
		DescriptorSet->Update();

		CmdList.BindDescriptorSets(1, &DescriptorSet);

		PipelineStateInitializer PSOInit = {};
		PSOInit.Viewport.Width = SceneDepth->Width;
		PSOInit.Viewport.Height = SceneDepth->Height;
		PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSOInit.DepthStencilState.DepthWriteEnable = false;
		PSOInit.GraphicsPipelineState.Vertex = *ShaderMapRef<FullscreenVS>();
		PSOInit.GraphicsPipelineState.Fragment = *ShaderMapRef<FullscreenFS<EVisualize::Depth>>();

		CmdList.BindPipeline(PSOInit);

		CmdList.Draw(3, 1, 0, 0);
	}

	/*RenderPassInitializer RPInit = { 1 };
	RPInit.ColorTargets[0] = SurfaceView;
	RPInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

	CmdList.BeginRenderPass(RPInit);

	drm::DescriptorSetRef DescriptorSet = drm::CreateDescriptorSet();
	DescriptorSet->Write(SceneDepth, SamplerState{ EFilter::Nearest }, ShaderBinding(0));
	DescriptorSet->Update();

	CmdList.BindDescriptorSets(1, &DescriptorSet);

	PipelineStateInitializer PSOInit = {};
	PSOInit.Viewport.Width = SceneDepth->Width;
	PSOInit.Viewport.Height = SceneDepth->Height;
	PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
	PSOInit.DepthStencilState.DepthWriteEnable = false;
	PSOInit.GraphicsPipelineState.Vertex = *ShaderMapRef<FullscreenVS>();
	PSOInit.GraphicsPipelineState.Fragment = *ShaderMapRef<FullscreenFS<EVisualize::Depth>>();

	CmdList.BindPipeline(PSOInit);

	CmdList.Draw(3, 1, 0, 0);*/

	CmdList.EndRenderPass();
}
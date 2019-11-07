#include "SceneRenderer.h"
#include "FullscreenQuad.h"
#include "RayMarching.h"
#include "Light.h"
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>

SceneRenderer::SceneRenderer(const Scene& Scene)
{
	gScreen.RegisterScreenResChangedCallback([&](int32 Width, int32 Height)
	{
		SceneDepth = drm::CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::RenderTargetable);
	});

	Cube = Scene.Assets.GetStaticMesh("Cube");

	gVoxelGridSize = Platform.GetInt("Engine.ini", "Renderer", "VoxelGridSize", 256);

	//VoxelColor = drm::CreateImage(gVoxelGridSize, gVoxelGridSize, gVoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage);
	VoxelColors = drm::CreateBuffer(EBufferUsage::Storage, gVoxelGridSize * gVoxelGridSize * gVoxelGridSize * sizeof(uint32) + sizeof(uint32));
	VoxelPositions = drm::CreateBuffer(EBufferUsage::Storage, gVoxelGridSize * gVoxelGridSize * gVoxelGridSize * sizeof(glm::vec3));

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
		drm::RenderTargetViewRef SurfaceView = drm::GetSurfaceView(ELoadAction::Clear, EStoreAction::Store, ClearColorValue{});
		drm::RenderTargetViewRef DepthView = drm::CreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{}, EImageLayout::DepthWriteStencilWrite);

		RenderPassInitializer RenderPassInit = { 1 };
		RenderPassInit.ColorTargets[0] = SurfaceView;
		RenderPassInit.DepthTarget = DepthView;
		RenderPassInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

		CmdList.BeginRenderPass(RenderPassInit);

		RenderLightingPass(Scene, CmdList);

		RenderSkybox(Scene, CmdList);

		CmdList.EndRenderPass();
	}

	CmdList.Finish();

	drm::SubmitCommands(CommandList);

	drm::EndFrame();
}

void SceneRenderer::RenderRayMarching(SceneProxy& Scene, RenderCommandList& CmdList)
{
	/*PipelineStateInitializer PSOInit = {};

	Ref<FullscreenVS> VertShader = *ShaderMapRef<FullscreenVS>();
	Ref<RayMarchingFS> FragShader = *ShaderMapRef<RayMarchingFS>();

	PSOInit.Viewport.Width = Screen.GetWidth();
	PSOInit.Viewport.Height = Screen.GetHeight();

	PSOInit.GraphicsPipelineState = { VertShader, nullptr, nullptr, nullptr, FragShader };

	CmdList.BindPipeline(PSOInit);

	Scene.SetResources(CmdList, FragShader, FragShader->SceneBindings);

	CmdList.Draw(3, 1, 0, 0);*/
}

void SceneRenderer::RenderLightingPass(SceneProxy& Scene, RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.Width = gScreen.GetWidth();
	PSOInit.Viewport.Height = gScreen.GetHeight();

	LightingPass::PassDescriptors Descriptors = { Scene.DescriptorSet };

	Scene.LightingPass[EStaticDrawListType::Opaque].Draw(CmdList, PSOInit, Descriptors);
	Scene.LightingPass[EStaticDrawListType::Masked].Draw(CmdList, PSOInit, Descriptors);
}
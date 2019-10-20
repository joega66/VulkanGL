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

	const glm::mat4 OrthoProj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 3.0f);

	std::array<glm::mat4, 3> VoxelOrthoProj;
	VoxelOrthoProj[0] = OrthoProj * glm::lookAt(glm::vec3(2, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	VoxelOrthoProj[1] = OrthoProj * glm::lookAt(glm::vec3(0, 2, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
	VoxelOrthoProj[2] = OrthoProj * glm::lookAt(glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	VoxelOrthoProjBuffer = drm::CreateUniformBuffer(VoxelOrthoProj.size() * sizeof(glm::mat4), VoxelOrthoProj.data(), EUniformUpdate::SingleFrame);

	Voxels = drm::CreateImage(gVoxelGridSize.x, gVoxelGridSize.y, gVoxelGridSize.x, EFormat::R8G8B8A8_UNORM, EImageUsage::UnorderedAccess);

	VoxelsDescriptorSet = drm::CreateDescriptorSet();
	VoxelsDescriptorSet->Write(VoxelOrthoProjBuffer, ShaderBinding(0));
	VoxelsDescriptorSet->Write(Voxels, ShaderBinding(1));
	VoxelsDescriptorSet->Update();
}

void SceneRenderer::Render(SceneProxy& Scene)
{
	drm::BeginFrame();

	RenderCommandListRef CommandList = drm::CreateCommandList();
	RenderCommandList& CmdList = *CommandList;

	{
		if (Platform.GetBool("Engine.ini", "Renderer", "RenderVoxels", false))
		{
			RenderVoxels(Scene, CmdList);
		}
		
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

	ColorBlendAttachmentState& BlendState = PSOInit.ColorBlendAttachmentStates[0];
	BlendState.BlendEnable = true;
	BlendState.SrcColorBlendFactor = EBlendFactor::SRC_ALPHA;
	BlendState.DstColorBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	BlendState.ColorBlendOp = EBlendOp::ADD;
	BlendState.SrcAlphaBlendFactor = EBlendFactor::SRC_ALPHA;
	BlendState.DstAlphaBlendFactor = EBlendFactor::ZERO;
	BlendState.AlphaBlendOp = EBlendOp::ADD;

	Scene.LightingPass[EStaticDrawListType::Masked].Draw(CmdList, PSOInit, Descriptors);
}
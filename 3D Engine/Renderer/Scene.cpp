#include "Scene.h"
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include "FullscreenQuad.h"
#include "RayMarching.h"
#include "Light.h"

Scene::Scene()
{
	Screen.RegisterScreenResChangedCallback([&](int32 Width, int32 Height)
	{
		SceneDepth = drm::CreateImage(Width, Height, EImageFormat::D16_UNORM, EResourceUsage::RenderTargetable);
		OutlineDepthStencil = drm::CreateImage(Width, Height, EImageFormat::D24_UNORM_S8_UINT, EResourceUsage::RenderTargetable);
	});
	
	Skybox = GAssetManager.GetCubemap("Engine-Cubemap-Default");
}

void Scene::Render()
{
	drm::BeginFrame();

	RenderCommandListRef CmdList = drm::CreateCommandList();

	{
		drm::RenderTargetViewRef SurfaceView = drm::GetSurfaceView(ELoadAction::Clear, EStoreAction::Store, ClearColorValue{});
		drm::RenderTargetViewRef DepthView = drm::CreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

		RenderPassInitializer RenderPassInit = { 1 };
		RenderPassInit.ColorTargets[0] = SurfaceView;
		RenderPassInit.DepthTarget = DepthView;
		RenderPassInit.DepthStencilTransition = EDepthStencilTransition::DepthWriteStencilWrite;
		RenderPassInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

		CmdList->BeginRenderPass(RenderPassInit);

		RenderLightingPass(*CmdList);
		RenderRayMarching(*CmdList);
		RenderLines(*CmdList);
		RenderSkybox(*CmdList);

		CmdList->EndRenderPass();
	}

	RenderOutlines(*CmdList);

	CmdList->Finish();

	drm::SubmitCommands(CmdList);

	drm::EndFrame();
}

void Scene::RenderRayMarching(RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	Ref<FullscreenVS> VertShader = *ShaderMapRef<FullscreenVS>();
	Ref<RayMarchingFS> FragShader = *ShaderMapRef<RayMarchingFS>();

	PSOInit.Viewport.Width = Screen.GetWidth();
	PSOInit.Viewport.Height = Screen.GetHeight();

	PSOInit.DepthStencilState.DepthTestEnable = true;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	PSOInit.GraphicsPipelineState = { VertShader, nullptr, nullptr, nullptr, FragShader };

	CmdList.BindPipeline(PSOInit);

	SetResources(CmdList, FragShader, FragShader->SceneBindings);

	CmdList.Draw(3, 1, 0, 0);
}

void Scene::RenderLightingPass(RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.Width = Screen.GetWidth();
	PSOInit.Viewport.Height = Screen.GetHeight();

	PSOInit.DepthStencilState.DepthTestEnable = true;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	LightingPass.Execute(CmdList, PSOInit, *this);
}

void Scene::RenderLines(RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.Width = Screen.GetWidth();
	PSOInit.Viewport.Height = Screen.GetHeight();

	PSOInit.DepthStencilState.DepthTestEnable = true;

	PSOInit.RasterizationState.CullMode = ECullMode::None;
	PSOInit.RasterizationState.PolygonMode = EPolygonMode::Line;

	Lines.Execute(CmdList, PSOInit, *this);
}

void Scene::RenderSkybox(RenderCommandList& CmdList)
{
	StaticMeshRef Cube = GAssetManager.GetStaticMesh("Cube");

	Ref<SkyboxVS> VertShader = *ShaderMapRef<SkyboxVS>();
	Ref<SkyboxFS> FragShader = *ShaderMapRef<SkyboxFS>();

	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.Width = Screen.GetWidth();
	PSOInit.Viewport.Height = Screen.GetHeight();

	PSOInit.DepthStencilState.DepthTestEnable = true;
	PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::LEqual;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	PSOInit.GraphicsPipelineState = { VertShader, nullptr, nullptr, nullptr, FragShader };

	CmdList.BindPipeline(PSOInit);

	CmdList.SetUniformBuffer(VertShader, VertShader->View, View.Uniform);
	CmdList.SetShaderImage(FragShader, FragShader->Skybox, Skybox, SamplerState{ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear });

	for (const auto& Element : Cube->Batch.Elements)
	{
		CmdList.BindVertexBuffers(0, Element.PositionBuffer);
		CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
	}
}

void Scene::RenderOutlines(RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	{
		drm::RenderTargetViewRef StencilView = CreateRenderTargetView(OutlineDepthStencil, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

		RenderPassInitializer RenderPassInit = {};
		RenderPassInit.DepthTarget = StencilView;
		RenderPassInit.DepthStencilTransition = EDepthStencilTransition::DepthWriteStencilWrite;
		RenderPassInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(OutlineDepthStencil->Width, OutlineDepthStencil->Height) };

		CmdList.BeginRenderPass(RenderPassInit);

		PSOInit.Viewport.Width = Screen.GetWidth();
		PSOInit.Viewport.Height = Screen.GetHeight();

		PSOInit.DepthStencilState.DepthTestEnable = true;
		PSOInit.DepthStencilState.StencilTestEnable = true;

		PSOInit.DepthStencilState.Back.CompareOp = ECompareOp::Always;
		PSOInit.DepthStencilState.Back.FailOp = EStencilOp::Replace;
		PSOInit.DepthStencilState.Back.DepthFailOp = EStencilOp::Replace;
		PSOInit.DepthStencilState.Back.PassOp = EStencilOp::Replace;
		PSOInit.DepthStencilState.Back.CompareMask = 0xff;
		PSOInit.DepthStencilState.Back.WriteMask = 0xff;
		PSOInit.DepthStencilState.Back.Reference = 1;

		Stencil.Execute(CmdList, PSOInit, *this);

		CmdList.EndRenderPass();
	}
	
	{
		drm::RenderTargetViewRef SurfaceView = drm::GetSurfaceView(ELoadAction::Load, EStoreAction::Store, ClearColorValue{});
		drm::RenderTargetViewRef OutlineView = drm::CreateRenderTargetView(OutlineDepthStencil, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{ 1.0f, 0 });

		RenderPassInitializer RenderPassInit = { 1 };
		RenderPassInit.ColorTargets[0] = SurfaceView;
		RenderPassInit.DepthTarget = OutlineView;
		RenderPassInit.DepthStencilTransition = EDepthStencilTransition::DepthWriteStencilWrite;
		RenderPassInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(OutlineDepthStencil->Width, OutlineDepthStencil->Height) };

		CmdList.BeginRenderPass(RenderPassInit);

		PSOInit.DepthStencilState.DepthTestEnable = false;
		PSOInit.DepthStencilState.DepthWriteEnable = false;

		PSOInit.DepthStencilState.Back.CompareOp = ECompareOp::NotEqual;
		PSOInit.DepthStencilState.Back.FailOp = EStencilOp::Keep;
		PSOInit.DepthStencilState.Back.DepthFailOp = EStencilOp::Keep;
		PSOInit.DepthStencilState.Back.PassOp = EStencilOp::Replace;
		PSOInit.DepthStencilState.Back.CompareMask = 0xff;
		PSOInit.DepthStencilState.Back.WriteMask = 0;
		PSOInit.DepthStencilState.Back.Reference = 1;

		Outline.Execute(CmdList, PSOInit, *this);

		CmdList.EndRenderPass();
	}
}

Scene& Scene::Get()
{
	static Scene Scene;
	return Scene;
}
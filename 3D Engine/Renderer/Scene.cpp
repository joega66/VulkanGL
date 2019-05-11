#include "Scene.h"
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include "FullscreenQuad.h"
#include "RayMarching.h"
#include "Light.h"

Scene::Scene()
{
	SceneDepth = drm::CreateImage((uint32)Screen.Width, (uint32)Screen.Height, IF_D32_SFLOAT, EResourceUsage::RenderTargetable);
	OutlineDepthStencil = drm::CreateImage((uint32)Screen.Width, (uint32)Screen.Height, IF_D32_SFLOAT_S8_UINT, EResourceUsage::RenderTargetable);
	Skybox = GAssetManager.GetCubemap("Engine-Cubemap-Default");
}

void Scene::Render()
{
	drm::BeginFrame();

	RenderCommandListRef CmdList = drm::CreateCommandList();

	//RenderRayMarching(*CmdList);
	RenderLightingPass(*CmdList);
	RenderEditorPrimitives(*CmdList);
	CmdList->Finish();

	drm::SubmitCommands(CmdList);

	// @todo Descriptor pools should be owned by the command list so they can be freed individually.
	drm::EndFrame();
}

void Scene::RenderRayMarching(RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	Ref<FullscreenVS> VertShader = *ShaderMapRef<FullscreenVS>();
	Ref<RayMarchingFS> FragShader = *ShaderMapRef<RayMarchingFS>();

	drm::RenderTargetViewRef SurfaceView = drm::GetSurfaceView(ELoadAction::Clear, EStoreAction::Store, { 0, 0, 0, 0 });
	drm::RenderTargetViewRef DepthView = drm::CreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	CmdList.SetRenderTargets(1, &SurfaceView, DepthView, EDepthStencilAccess::DepthWriteStencilWrite);

	PSOInit.Viewport.X = 0.0f;
	PSOInit.Viewport.Y = 0.0f;
	PSOInit.Viewport.Width = Screen.Width;
	PSOInit.Viewport.Height = Screen.Height;

	PSOInit.DepthStencilState.DepthTestEnable = true;

	PSOInit.ColorBlendAttachmentStates[0].ColorWriteMask = EColorChannel::RGBA;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	PSOInit.GraphicsPipelineState = { VertShader, nullptr, nullptr, nullptr, FragShader };

	CmdList.SetPipelineState(PSOInit);

	CmdList.SetUniformBuffer(FragShader, FragShader->View, View.Uniform);
	CmdList.SetStorageBuffer(FragShader, FragShader->LightBuffer, LightBuffer);

	CmdList.Draw(3, 1, 0, 0);
}

void Scene::RenderLightingPass(RenderCommandList& CmdList)
{
	drm::RenderTargetViewRef SurfaceView = drm::GetSurfaceView(ELoadAction::Clear, EStoreAction::Store, { 0, 0, 0, 0 });
	drm::RenderTargetViewRef DepthView = drm::CreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	CmdList.SetRenderTargets(1, &SurfaceView, DepthView, EDepthStencilAccess::DepthWriteStencilWrite);

	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.X = 0.0f;
	PSOInit.Viewport.Y = 0.0f;
	PSOInit.Viewport.Width = Screen.Width;
	PSOInit.Viewport.Height = Screen.Height;

	PSOInit.DepthStencilState.DepthTestEnable = true;

	PSOInit.ColorBlendAttachmentStates[0].ColorWriteMask = EColorChannel::RGBA;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	LightingPassDrawingPlans.Execute(CmdList, PSOInit, View);
}

void Scene::RenderEditorPrimitives(RenderCommandList& CmdList)
{
	RenderLines(CmdList);
	RenderSkybox(CmdList);
	RenderOutlines(CmdList);
}

void Scene::RenderLines(RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.X = 0.0f;
	PSOInit.Viewport.Y = 0.0f;
	PSOInit.Viewport.Width = Screen.Width;
	PSOInit.Viewport.Height = Screen.Height;

	PSOInit.DepthStencilState.DepthTestEnable = true;

	PSOInit.ColorBlendAttachmentStates[0].ColorWriteMask = EColorChannel::RGBA;

	PSOInit.RasterizationState.CullMode = ECullMode::None;
	PSOInit.RasterizationState.PolygonMode = EPolygonMode::Line;

	Lines.Execute(CmdList, PSOInit, View);
}

void Scene::RenderSkybox(RenderCommandList& CmdList)
{
	StaticMeshRef Cube = GAssetManager.GetStaticMesh("Cube");

	Ref<SkyboxVS> VertShader = *ShaderMapRef<SkyboxVS>();
	Ref<SkyboxFS> FragShader = *ShaderMapRef<SkyboxFS>();

	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.X = 0.0f;
	PSOInit.Viewport.Y = 0.0f;
	PSOInit.Viewport.Width = Screen.Width;
	PSOInit.Viewport.Height = Screen.Height;

	PSOInit.DepthStencilState.DepthTestEnable = true;
	PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::LEqual;

	PSOInit.ColorBlendAttachmentStates[0].ColorWriteMask = EColorChannel::RGBA;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	PSOInit.GraphicsPipelineState = { VertShader, nullptr, nullptr, nullptr, FragShader };

	CmdList.SetPipelineState(PSOInit);

	CmdList.SetUniformBuffer(VertShader, VertShader->View, View.Uniform);
	CmdList.SetShaderImage(FragShader, FragShader->Skybox, Skybox, SamplerState{ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear });

	for (const auto& Resource : Cube->Resources)
	{
		CmdList.SetVertexStream(0, Resource.PositionBuffer);
		CmdList.DrawIndexed(Resource.IndexBuffer, Resource.IndexCount, 1, 0, 0, 0);
	}
}

void Scene::RenderOutlines(RenderCommandList& CmdList)
{
	drm::RenderTargetViewRef StencilView = CreateRenderTargetView(OutlineDepthStencil, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	CmdList.SetRenderTargets(0, nullptr, StencilView, EDepthStencilAccess::DepthWriteStencilWrite);

	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.X = 0.0f;
	PSOInit.Viewport.Y = 0.0f;
	PSOInit.Viewport.Width = Screen.Width;
	PSOInit.Viewport.Height = Screen.Height;

	PSOInit.DepthStencilState.DepthTestEnable = true;
	PSOInit.DepthStencilState.StencilTestEnable = true;

	PSOInit.DepthStencilState.Back.CompareOp = ECompareOp::Always;
	PSOInit.DepthStencilState.Back.FailOp = EStencilOp::Replace;
	PSOInit.DepthStencilState.Back.DepthFailOp = EStencilOp::Replace;
	PSOInit.DepthStencilState.Back.PassOp = EStencilOp::Replace;
	PSOInit.DepthStencilState.Back.CompareMask = 0xff;
	PSOInit.DepthStencilState.Back.WriteMask = 0xff;
	PSOInit.DepthStencilState.Back.Reference = 1;

	Stencil.Execute(CmdList, PSOInit, View);

	drm::RenderTargetViewRef SurfaceView = drm::GetSurfaceView(ELoadAction::Load, EStoreAction::Store, { 0, 0, 0, 0 });
	drm::RenderTargetViewRef OutlineView = drm::CreateRenderTargetView(OutlineDepthStencil, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{ 1.0f, 0 });

	CmdList.SetRenderTargets(1, &SurfaceView, OutlineView, EDepthStencilAccess::DepthWriteStencilWrite);

	PSOInit.DepthStencilState.DepthTestEnable = false;
	PSOInit.DepthStencilState.DepthWriteEnable = false;

	PSOInit.DepthStencilState.Back.CompareOp = ECompareOp::NotEqual;
	PSOInit.DepthStencilState.Back.FailOp = EStencilOp::Keep;
	PSOInit.DepthStencilState.Back.DepthFailOp = EStencilOp::Keep;
	PSOInit.DepthStencilState.Back.PassOp = EStencilOp::Replace;
	PSOInit.DepthStencilState.Back.CompareMask = 0xff;
	PSOInit.DepthStencilState.Back.WriteMask = 0;
	PSOInit.DepthStencilState.Back.Reference = 1;

	Outline.Execute(CmdList, PSOInit, View);
}

Scene& Scene::Get()
{
	static Scene Scene;
	return Scene;
}
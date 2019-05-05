#include "Scene.h"
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include "FullscreenQuad.h"
#include "RayMarching.h"
#include "Light.h"

Scene::Scene()
{
	SceneDepth = GLCreateImage((uint32)Screen.Width, (uint32)Screen.Height, IF_D32_SFLOAT, EResourceUsage::RenderTargetable);
	OutlineDepthStencil = GLCreateImage((uint32)Screen.Width, (uint32)Screen.Height, IF_D32_SFLOAT_S8_UINT, EResourceUsage::RenderTargetable);
	Skybox = GAssetManager.GetCubemap("Engine-Cubemap-Default");
}

void Scene::Render()
{
	RenderCommandListRef CmdListPtr = GDRM->BeginFrame();
	RenderCommandList& CmdList = *CmdListPtr;

	//RenderRayMarching();
	RenderLightingPass(CmdList);
	RenderEditorPrimitives(CmdList);

	CmdList.Finish();

	GDRM->EndFrame(CmdListPtr);
}

void Scene::RenderRayMarching(RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	GLShaderRef VertShader = GLCreateShader<FullscreenVS>();
	GLShaderRef FragShader = GLCreateShader<RayMarchingFS>();

	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Clear, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef DepthView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	CmdList.SetRenderTargets(1, &SurfaceView, DepthView, EDepthStencilAccess::DepthWrite);
	
	PSOInit.Viewport.X = 0.0f;
	PSOInit.Viewport.Y = 0.0f;
	PSOInit.Viewport.Width = Screen.Width;
	PSOInit.Viewport.Height = Screen.Height;

	PSOInit.DepthStencilState.DepthTestEnable = true;

	PSOInit.ColorBlendAttachmentStates[0].ColorWriteMask = EColorChannel::RGBA;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	CmdList.SetPipelineState(PSOInit);

	CmdList.SetGraphicsPipeline(VertShader, nullptr, nullptr, nullptr, FragShader);

	CmdList.SetUniformBuffer(FragShader, FragShader->GetUniformLocation("ViewUniform"), View.Uniform);
	CmdList.SetStorageBuffer(FragShader, FragShader->GetUniformLocation("LightBuffer"), LightBuffer);

	CmdList.Draw(3, 1, 0, 0);
}

void Scene::RenderLightingPass(RenderCommandList& CmdList)
{
	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Clear, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef DepthView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	CmdList.SetRenderTargets(1, &SurfaceView, DepthView, EDepthStencilAccess::DepthWrite);

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

	GLShaderRef VertShader = GLCreateShader<SkyboxVS>();
	GLShaderRef FragShader = GLCreateShader<SkyboxFS>();

	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.X = 0.0f;
	PSOInit.Viewport.Y = 0.0f;
	PSOInit.Viewport.Width = Screen.Width;
	PSOInit.Viewport.Height = Screen.Height;

	PSOInit.DepthStencilState.DepthTestEnable = true;
	PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::LEqual;

	PSOInit.ColorBlendAttachmentStates[0].ColorWriteMask = EColorChannel::RGBA;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	CmdList.SetPipelineState(PSOInit);

	CmdList.SetGraphicsPipeline(VertShader, nullptr, nullptr, nullptr, FragShader);

	CmdList.SetUniformBuffer(VertShader, VertShader->GetUniformLocation("ViewUniform"), View.Uniform);
	CmdList.SetShaderImage(FragShader, FragShader->GetUniformLocation("Skybox"), Skybox, SamplerState{EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear});

	for (const auto& Resource : Cube->Resources)
	{
		CmdList.SetVertexStream(VertShader->GetAttributeLocation("Position"), Resource.PositionBuffer);
		CmdList.DrawIndexed(Resource.IndexBuffer, Resource.IndexCount, 1, 0, 0, 0);
	}
}

void Scene::RenderOutlines(RenderCommandList& CmdList)
{
	GLRenderTargetViewRef StencilView = GLCreateRenderTargetView(OutlineDepthStencil, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

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

	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Load, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef OutlineView = GLCreateRenderTargetView(OutlineDepthStencil, ELoadAction::Load, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	CmdList.SetRenderTargets(1, &SurfaceView, OutlineView, EDepthStencilAccess::StencilWrite);

	PSOInit.DepthStencilState.DepthTestEnable = false;

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
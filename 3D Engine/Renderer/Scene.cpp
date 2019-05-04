#include "Scene.h"
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include "FullscreenQuad.h"
#include "RayMarching.h"
#include "Light.h"

// @todo-joe All functions that don't require a command list should be wrapped in graphics utils class.

Scene::Scene()
{
	SceneDepth = GLCreateImage((uint32)Screen.Width, (uint32)Screen.Height, IF_D32_SFLOAT, EResourceUsage::RenderTargetable);
	OutlineDepthStencil = GLCreateImage((uint32)Screen.Width, (uint32)Screen.Height, IF_D32_SFLOAT_S8_UINT, EResourceUsage::RenderTargetable);
	Skybox = GAssetManager.GetCubemap("Engine-Cubemap-Default");
}

void Scene::Render()
{
	GLBeginRender();

	//RenderRayMarching();
	RenderLightingPass();
	RenderEditorPrimitives();

	GLEndRender();
}

void Scene::RenderRayMarching()
{
	PipelineStateInitializer PSOInit = {};

	GLShaderRef VertShader = GLCreateShader<FullscreenVS>();
	GLShaderRef FragShader = GLCreateShader<RayMarchingFS>();

	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Clear, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef DepthView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	GLSetRenderTargets(1, &SurfaceView, DepthView, EDepthStencilAccess::DepthWrite);
	
	PSOInit.Viewport.X = 0.0f;
	PSOInit.Viewport.Y = 0.0f;
	PSOInit.Viewport.Width = Screen.Width;
	PSOInit.Viewport.Height = Screen.Height;

	PSOInit.DepthStencilState.DepthTestEnable = true;

	PSOInit.ColorBlendAttachmentStates[0].ColorWriteMask = EColorChannel::RGBA;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	GRenderCmdList->SetPipelineState(PSOInit);

	GLSetGraphicsPipeline(VertShader, nullptr, nullptr, nullptr, FragShader);

	GLSetUniformBuffer(FragShader, "ViewUniform", View.Uniform);
	GLSetStorageBuffer(FragShader, FragShader->GetUniformLocation("LightBuffer"), LightBuffer);

	GLDraw(3, 1, 0, 0);
}

void Scene::RenderLightingPass()
{
	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Clear, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef DepthView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	GLSetRenderTargets(1, &SurfaceView, DepthView, EDepthStencilAccess::DepthWrite);

	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.X = 0.0f;
	PSOInit.Viewport.Y = 0.0f;
	PSOInit.Viewport.Width = Screen.Width;
	PSOInit.Viewport.Height = Screen.Height;

	PSOInit.DepthStencilState.DepthTestEnable = true;

	PSOInit.ColorBlendAttachmentStates[0].ColorWriteMask = EColorChannel::RGBA;

	PSOInit.RasterizationState.CullMode = ECullMode::None;

	LightingPassDrawingPlans.Execute(PSOInit, View);
}

void Scene::RenderEditorPrimitives()
{
	RenderLines();
	RenderSkybox();
	RenderOutlines();
}

void Scene::RenderLines()
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

	Lines.Execute(PSOInit, View);
}

void Scene::RenderSkybox()
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

	GRenderCmdList->SetPipelineState(PSOInit);

	GLSetGraphicsPipeline(VertShader, nullptr, nullptr, nullptr, FragShader);

	GLSetUniformBuffer(VertShader, VertShader->GetUniformLocation("ViewUniform"), View.Uniform);
	GLSetShaderImage(FragShader, FragShader->GetUniformLocation("Skybox"), Skybox, SamplerState{EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear});

	for (const auto& Resource : Cube->Resources)
	{
		GLSetVertexStream(VertShader->GetAttributeLocation("Position"), Resource.PositionBuffer);
		GLDrawIndexed(Resource.IndexBuffer, Resource.IndexCount, 1, 0, 0, 0);
	}
}

void Scene::RenderOutlines()
{
	GLRenderTargetViewRef StencilView = GLCreateRenderTargetView(OutlineDepthStencil, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	GLSetRenderTargets(0, nullptr, StencilView, EDepthStencilAccess::DepthWriteStencilWrite);

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

	Stencil.Execute(PSOInit, View);

	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Load, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef OutlineView = GLCreateRenderTargetView(OutlineDepthStencil, ELoadAction::Load, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	GLSetRenderTargets(1, &SurfaceView, OutlineView, EDepthStencilAccess::StencilWrite);

	PSOInit.DepthStencilState.DepthTestEnable = false;

	PSOInit.DepthStencilState.Back.CompareOp = ECompareOp::NotEqual;
	PSOInit.DepthStencilState.Back.FailOp = EStencilOp::Keep;
	PSOInit.DepthStencilState.Back.DepthFailOp = EStencilOp::Keep;
	PSOInit.DepthStencilState.Back.PassOp = EStencilOp::Replace;
	PSOInit.DepthStencilState.Back.CompareMask = 0xff;
	PSOInit.DepthStencilState.Back.WriteMask = 0;
	PSOInit.DepthStencilState.Back.Reference = 1;

	GRenderCmdList->SetPipelineState(PSOInit);

	Outline.Execute(PSOInit, View);
}

Scene& Scene::Get()
{
	static Scene Scene;
	return Scene;
}
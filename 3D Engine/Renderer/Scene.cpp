#include "Scene.h"
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include "FullscreenQuad.h"
#include "RayMarching.h"

Scene::Scene()
{
	SceneDepth = GLCreateImage(Screen.Width, Screen.Height, IF_D32_SFLOAT, EResourceUsage::RenderTargetable);
	OutlineDepthStencil = GLCreateImage(Screen.Width, Screen.Height, IF_D32_SFLOAT_S8_UINT, EResourceUsage::RenderTargetable);

	Skybox = GAssetManager.GetCubemap("Engine-Cubemap-Default");
}

void Scene::Render()
{
	GLBeginRender();

	RenderRayMarching();
	RenderLightingPass();
	RenderEditorPrimitives();

	GLEndRender();
}

void Scene::RenderRayMarching()
{
	GLShaderRef VertShader = GLCreateShader<FullscreenVS>();
	GLShaderRef FragShader = GLCreateShader<RayMarchingFS>();

	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Clear, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef DepthView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	GLSetRenderTargets(1, &SurfaceView, DepthView, EDepthStencilAccess::DepthWrite);
	GLSetDepthTest(true);
	GLSetGraphicsPipeline(VertShader, nullptr, nullptr, nullptr, FragShader);
	GLSetUniformBuffer(FragShader, "ViewUniform", View.Uniform);

	GLDraw(3, 1, 0, 0);
}

void Scene::RenderLightingPass()
{
	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Load, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef DepthView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Load, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	GLSetRenderTargets(1, &SurfaceView, DepthView, EDepthStencilAccess::DepthWrite);
	GLSetViewport(0.0f, 0.0f, (float)Screen.Width, (float)Screen.Height);
	GLSetDepthTest(true);
	GLSetColorMask(0, EColorChannel::RGBA);
	GLSetRasterizerState(ECullMode::None);

	LightingPassDrawingPlans.Execute(View);
}

void Scene::RenderEditorPrimitives()
{
	RenderLines();
	RenderSkybox();
	RenderOutlines();
}

void Scene::RenderLines()
{
	Lines.Execute(View);
	GLSetRasterizerState(ECullMode::None);
}

void Scene::RenderSkybox()
{
	GLShaderRef VertShader = GLCreateShader<SkyboxVS>();
	GLShaderRef FragShader = GLCreateShader<SkyboxFS>();

	StaticMeshRef Cube = GAssetManager.GetStaticMesh("Cube");

	GLSetDepthTest(true, EDepthCompareTest::LEqual);
	GLSetGraphicsPipeline(VertShader, nullptr, nullptr, nullptr, FragShader);
	GLSetUniformBuffer(VertShader, VertShader->GetUniformLocation("ViewUniform"), View.Uniform);
	GLSetShaderImage(FragShader, FragShader->GetUniformLocation("Skybox"), Skybox, SamplerState{EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear});

	for (const auto& Resource : Cube->Resources)
	{
		GLSetVertexStream(VertShader->GetAttributeLocation("Position"), Resource.PositionBuffer);
		GLDrawIndexed(Resource.IndexBuffer, Resource.IndexCount, 1, 0, 0, 0);
	}

	GLSetDepthTest(true);
}

void Scene::RenderOutlines()
{
	GLRenderTargetViewRef StencilView = GLCreateRenderTargetView(OutlineDepthStencil, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	GLSetRenderTargets(0, nullptr, StencilView, EDepthStencilAccess::DepthWriteStencilWrite);
	GLSetStencilTest(true);
	GLSetStencilState(ECompareOp::Always, EStencilOp::Replace, EStencilOp::Replace, EStencilOp::Replace, 0xff, 0xff, 1);

	Stencil.Execute(View);

	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Load, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef OutlineView = GLCreateRenderTargetView(OutlineDepthStencil, ELoadAction::Load, EStoreAction::Store, ClearDepthStencilValue{ 1.0f, 0 });

	GLSetRenderTargets(1, &SurfaceView, OutlineView, EDepthStencilAccess::StencilWrite);
	GLSetDepthTest(false);
	GLSetStencilState(ECompareOp::NotEqual, EStencilOp::Keep, EStencilOp::Keep, EStencilOp::Replace, 0xff, 0, 1);

	Outline.Execute(View);

	GLSetStencilTest(false);
	GLSetDepthTest(true);
}

Scene& Scene::Get()
{
	static Scene Scene;
	return Scene;
}
#include "Scene.h"

Scene::Scene()
{
	SceneDepth = GLCreateImage(GPlatform->GetWindowSize().x, GPlatform->GetWindowSize().y, IF_D32_SFLOAT, RU_RenderTargetable);
	SceneDepthView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, 1.0f, 0);

	HighlightDepthStencil = GLCreateImage(GPlatform->GetWindowSize().x, GPlatform->GetWindowSize().y, IF_D32_SFLOAT_S8_UINT, RU_RenderTargetable);
	HighlightDepthStencilView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, 1.0f, 0);
}

void Scene::Render()
{
	RenderLightingPass();
	RenderEditorPrimitives();
}

void Scene::RenderLightingPass()
{
	std::array<float, 4> ClearColor = { 0, 0, 0, 0 };
	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Clear, EStoreAction::Store, ClearColor);

	GLSetRenderTargets(1, &SurfaceView, SceneDepthView, DS_DepthWrite);
	GLSetViewport(0.0f, 0.0f, (float)GPlatform->GetWindowSize().x, (float)GPlatform->GetWindowSize().y);
	GLSetDepthTest(true);
	GLSetColorMask(0, Color_RGBA);
	GLSetRasterizerState(CM_None);

	LightingPassDrawingPlans.Execute(View);
}

void Scene::RenderEditorPrimitives()
{
	//GLSetRenderTargets(0, nullptr, HighlightDepthStencilView, DS_DepthWriteStencilWrite);
	//GLSetRasterizerState(CM_None);
	//GLSetDepthTest(true);
	//GLSetStencilTest(true);
	//GLSetStencilState(ECompareOp::Always, EStencilOp::Replace, EStencilOp::Replace, EStencilOp::Replace, 0xff, 0xff, 1);

	//Highlighted.Prepass.Execute(View);

	//GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Load, EStoreAction::Store, { 0, 0, 0, 0 });
	//GLSetRenderTargets(1, &SurfaceView, nullptr, DS_None);
	//GLSetDepthTest(false);
	//GLSetStencilState(ECompareOp::NotEqual, EStencilOp::Keep, EStencilOp::Keep, EStencilOp::Replace, 0xff, 0xff, 1);

	//Highlighted.Final.Execute(View);
}
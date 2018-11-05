#include "Scene.h"

Scene::Scene()
{
	SceneDepth = GLCreateImage(GPlatform->GetWindowSize().x, GPlatform->GetWindowSize().y, IF_D32_SFLOAT, RU_RenderTargetable);
	SceneDepthView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, 1.0f, 0);
}

void Scene::Render(const View& View)
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
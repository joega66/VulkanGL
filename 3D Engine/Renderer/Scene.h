#pragma once
#include "LightingPass.h"
#include "EditorPrimitives.h"

class Scene
{
public:
	View View;

	DrawingPlanList<LightingPassDrawingPlan> LightingPassDrawingPlans;
	DrawingPlanList<DepthPassDrawingPlan> Stencil;
	DrawingPlanList<OutlineDrawingPlan> Outline;

	GLImageRef Skybox;

	Scene();
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Render();

	static Scene& Get();

private:
	GLImageRef SceneDepth;
	GLImageRef OutlineDepthStencil;

	void RenderLightingPass();

	void RenderEditorPrimitives();
	void RenderSkybox();
	void RenderOutlines();
};
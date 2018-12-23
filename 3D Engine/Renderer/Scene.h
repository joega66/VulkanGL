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

	Scene();
	void Render();

private:
	GLImageRef SceneDepth;
	GLRenderTargetViewRef SceneDepthView;
	GLImageRef OutlineDepthStencil;

	void RenderLightingPass();
	void RenderEditorPrimitives();
	void RenderOutlines();
};
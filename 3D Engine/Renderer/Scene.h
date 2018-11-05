#pragma once
#include "LightingPass.h"

class Scene
{
public:
	DrawingPlanList<LightingPassDrawingPlan> LightingPassDrawingPlans;

	Scene();
	void Render(const View& View);

private:
	GLImageRef SceneDepth;
	GLRenderTargetViewRef SceneDepthView;
};
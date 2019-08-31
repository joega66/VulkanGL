#pragma once
#include <Engine/View.h>
#include "Light.h"
#include "LightingPass.h"
#include "EditorPrimitives.h"

class Scene
{
public:
	Scene();
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	View View;

	DrawingPlanList<LightingPassDrawingPlan> LightingPass;
	DrawingPlanList<DepthPassDrawingPlan> Stencil;
	DrawingPlanList<OutlineDrawingPlan> Outline;
	DrawingPlanList<LineDrawingPlan> Lines;

	std::vector<PointLightProxy> PointLightProxies;

	drm::ImageRef Skybox;
};
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

	DrawList<LightingPassDrawPlan> LightingPass;
	DrawList<DepthPassDrawPlan> Stencil;
	DrawList<OutlineDrawPlan> Outline;
	DrawList<LineDrawPlan> Lines;

	std::vector<PointLightProxy> PointLightProxies;

	drm::ImageRef Skybox;
};
#pragma once
#include <Engine/View.h>
#include <ECS/EntityManager.h>
#include "Light.h"
#include "LightingPass.h"
#include "EditorPrimitives.h"

class Scene
{
	friend class CoreEngine;
	Scene();
public:
	EntityManager ECS;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
	
	View View;

	// @todo Move these to a renderer version of Scene class.
	DrawList<LightingPassDrawPlan> LightingPass;
	DrawList<DepthPassDrawPlan> Stencil;
	DrawList<OutlineDrawPlan> Outline;
	DrawList<LineDrawPlan> Lines;

	std::vector<PointLightProxy> PointLightProxies;

	drm::ImageRef Skybox;
};
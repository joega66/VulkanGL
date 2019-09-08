#pragma once
#include <Engine/View.h>
#include <ECS/EntityManager.h>

class Scene
{
	friend class CoreEngine;
	Scene();

public:
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	EntityManager ECS;

	View View;

	drm::ImageRef Skybox;
};
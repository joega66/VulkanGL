#pragma once
#include <ECS/EntityManager.h>
#include <Engine/View.h>
#include <Engine/AssetManager.h>

class Scene
{
	friend class CoreEngine;
	Scene();

public:
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	AssetManager Assets;

	EntityManager ECS;

	View View;

	drm::ImageRef Skybox;
};
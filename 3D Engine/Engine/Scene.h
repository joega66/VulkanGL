#pragma once
#include <ECS/EntityManager.h>
#include <Engine/View.h>
#include <Engine/AssetManager.h>

class DRMShaderMap;

class Scene
{
	friend class CoreEngine;
	Scene(DRMShaderMap& ShaderMap);

public:
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	DRMShaderMap& ShaderMap;

	AssetManager Assets;

	EntityManager ECS;

	View View;

	drm::ImageRef Skybox;
};
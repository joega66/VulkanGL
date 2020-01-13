#pragma once
#include <ECS/EntityManager.h>
#include <Engine/View.h>
#include <Engine/AssetManager.h>

class DRMShaderMap;

class Scene
{
	friend class EngineMain;
	Scene(class DRM& Device, DRMShaderMap& ShaderMap, class Screen& Screen);

public:
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	DRMShaderMap& ShaderMap;

	AssetManager Assets;

	EntityManager ECS;

	View View;

	drm::ImageRef Skybox;
};
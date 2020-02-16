#pragma once
#include <DRM.h>

class Scene
{
	friend class Engine;
	Scene(class Engine& Engine);

public:
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	drm::ImageRef Skybox;
};
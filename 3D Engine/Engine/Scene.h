#pragma once
#include <ECS/EntityManager.h>
#include <Engine/Camera.h>
#include <Engine/AssetManager.h>

class DRM;
class DRMShaderMap;
class Cursor;
class Input;
class Screen;

/** 
 * The Scene is the "root" of ECS. 
 * @todo Really, we don't need a Scene class. All its members could just be singleton components 
 * belonging to a "scene entity".
*/
class Scene
{
	friend class EngineMain;
	Scene(
		DRM& Device, 
		DRMShaderMap& ShaderMap, 
		Cursor& Cursor,
		Input& Input, 
		Screen& Screen
	);

public:
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	DRMShaderMap& ShaderMap;

	Cursor& Cursor;

	Input& Input;

	AssetManager Assets;

	EntityManager ECS;

	Camera Camera;

	drm::ImageRef Skybox;
};
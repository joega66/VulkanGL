#pragma once
#include <Platform/Platform.h>
#include <ECS/EntityManager.h>
#include "AssetManager.h"
#include "Scene.h"
#include "Camera.h"

class Cursor;
class Input;
class Screen;
class DRMDevice;
class DRMShaderMap;
namespace drm { class Surface; }

/** The main() of the engine, after the platform-dependent stuff has been resolved. */
class Engine
{
public:
	Engine(
		Platform& InPlatform,
		Cursor& InCursor,
		Input& InInput,
		Screen& InScreen,
		DRMDevice& InDevice,
		DRMShaderMap& InShaderMap,
		drm::Surface& InSurface
	);

	void Main();

	/** Platform implementations. */
	Platform& _Platform;
	Cursor& _Cursor;
	Input& _Input;
	Screen& _Screen;

	/** DRM implementations. */
	DRMDevice& Device;
	DRMShaderMap& ShaderMap;
	drm::Surface& Surface;

	/** Engine misc. */
	EntityManager ECS;
	AssetManager Assets;
	Scene Scene;
	Camera Camera;
};
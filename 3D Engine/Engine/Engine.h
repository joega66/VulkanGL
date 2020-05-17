#pragma once
#include <Platform/Platform.h>
#include <ECS/EntityManager.h>
#include "AssetManager.h"
#include "Camera.h"

class Cursor;
class Input;
class Screen;
class drm::Device;
namespace drm { class Surface; class ShaderLibrary; }

/** The main() of the engine, after the platform-dependent stuff has been resolved. */
class Engine
{
public:
	Engine(
		Platform& InPlatform,
		Cursor& InCursor,
		Input& InInput,
		Screen& InScreen,
		drm::Device& InDevice,
		drm::ShaderLibrary& InShaderLibrary,
		drm::Surface& InSurface
	);

	void Main();

	/** Platform implementations. */
	Platform& _Platform;
	Cursor& _Cursor;
	Input& _Input;
	Screen& _Screen;
	
	/** DRM implementations. */
	drm::Device& Device;
	drm::ShaderLibrary& ShaderLibrary;
	drm::Surface& Surface;

	/** Engine misc. */
	EntityManager ECS;
	AssetManager Assets;
	Camera Camera;
};
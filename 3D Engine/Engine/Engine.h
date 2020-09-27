#pragma once
#include <Platform/Platform.h>
#include <ECS/EntityManager.h>
#include "AssetManager.h"
#include "Camera.h"

class Cursor;
class Input;
class Screen;
namespace gpu { class Device; class Surface; class ShaderLibrary; }

/** The main() of the engine, after the platform-dependent stuff has been resolved. */
class Engine
{
public:
	Engine(
		Platform& InPlatform,
		Cursor& InCursor,
		Input& InInput,
		Screen& InScreen,
		gpu::Device& InDevice,
		gpu::ShaderLibrary& InShaderLibrary,
		gpu::Surface& InSurface
	);

	void Main();

	/** Platform implementations. */
	Platform& _Platform;
	Cursor& _Cursor;
	Input& _Input;
	Screen& _Screen;
	
	/** GPU/GPU.h implementations. */
	gpu::Device& _Device;
	gpu::ShaderLibrary& ShaderLibrary;
	gpu::Surface& Surface;

	/** Engine misc. */
	EntityManager _ECS;
	AssetManager Assets;
};
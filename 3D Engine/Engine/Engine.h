#pragma once
#include <Platform/Platform.h>
#include <ECS/EntityManager.h>
#include "AssetManager.h"
#include "Components/Camera.h"

class Cursor;
class Input;
class Screen;
namespace gpu { class Device; class Compositor; }

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
		gpu::Compositor& compositor
	);

	void Main();

	/** Platform implementations. */
	Platform& _Platform;
	Cursor& _Cursor;
	Input& _Input;
	Screen& _Screen;
	
	/** GPU/GPU.h implementations. */
	gpu::Device& _Device;
	gpu::Compositor& _Compositor;

	/** Engine misc. */
	EntityManager _ECS;
	AssetManager Assets;
};
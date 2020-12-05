#include <Renderer/SceneRenderer.h>
#include <Renderer/CameraRender.h>
#include <Systems/EditorControllerSystem.h>
#include <Systems/SceneSystem.h>
#include <Systems/SurfaceSystem.h>
#include <Systems/UserInterface.h>
#include <Systems/CameraSystem.h>
#include <Systems/ShadowSystem.h>
#include <Engine/Screen.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include "Engine.h"

Engine::Engine(
	Platform& InPlatform, 
	Cursor& InCursor,
	Input& InInput,
	Screen& InScreen,
	gpu::Device& InDevice,
	gpu::Compositor& compositor
) : _Platform(InPlatform)
	, _Cursor(InCursor)
	, _Input(InInput)
	, _Screen(InScreen)
	, _Device(InDevice)
	, _Compositor(compositor)
	, Assets(InDevice)
{
}

void Engine::Main()
{
	SystemsManager SystemsManager;

	SurfaceSystem surfaceSystem;
	SystemsManager.Register(surfaceSystem);

	CameraSystem cameraSystem;
	SystemsManager.Register(cameraSystem);

	ShadowSystem shadowSystem;
	SystemsManager.Register(shadowSystem);

	EditorControllerSystem EditorControllerSystem;
	SystemsManager.Register(EditorControllerSystem);

	SceneSystem SceneSystem;
	SystemsManager.Register(SceneSystem);

	UserInterface UserInterface;
	SystemsManager.Register(UserInterface);

	SystemsManager.StartRenderSystems(*this);
	SystemsManager.StartSystems(*this);

	SceneRenderer sceneRenderer(*this);

	while (!_Platform.WindowShouldClose())
	{
		_Platform.PollEvents();

		_Screen.CallEvents();

		_ECS.NotifyComponentEvents();

		SystemsManager.UpdateSystems(*this);
		SystemsManager.UpdateRenderSystems(*this);

		sceneRenderer.Render();

		_Cursor.Update(_Platform);
		_Input.Update(_Platform);

		// @todo Move me.
		for (auto entity : _ECS.GetEntities<Camera>())
		{
			auto& camera = _ECS.GetComponent<Camera>(entity);
			camera.SaveState();
		}
	}
}
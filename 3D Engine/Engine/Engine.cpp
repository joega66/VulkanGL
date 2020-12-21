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
	Platform& platform, 
	Cursor& cursor,
	Input& input,
	Screen& screen,
	gpu::Device& device,
	gpu::Compositor& compositor) 
	: _Platform(platform)
	, _Cursor(cursor)
	, _Input(input)
	, _Screen(screen)
	, _Device(device)
	, _Compositor(compositor)
	, _Assets(device)
{
}

void Engine::Main()
{
	SystemsManager systemsManager;

	SurfaceSystem surfaceSystem;
	systemsManager.Register(surfaceSystem);

	CameraSystem cameraSystem;
	systemsManager.Register(cameraSystem);

	ShadowSystem shadowSystem;
	systemsManager.Register(shadowSystem);

	EditorControllerSystem editorControllerSystem;
	systemsManager.Register(editorControllerSystem);

	SceneSystem sceneSystem;
	systemsManager.Register(sceneSystem);

	UserInterface userInterface;
	systemsManager.Register(userInterface);

	systemsManager.StartRenderSystems(*this);
	systemsManager.StartSystems(*this);

	SceneRenderer sceneRenderer(*this);

	while (!_Platform.WindowShouldClose())
	{
		_Platform.PollEvents();

		_Screen.CallEvents();

		_ECS.NotifyComponentEvents();

		systemsManager.UpdateSystems(*this);
		systemsManager.UpdateRenderSystems(*this);

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
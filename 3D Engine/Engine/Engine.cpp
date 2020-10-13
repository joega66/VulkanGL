#include <Renderer/SceneRenderer.h>
#include <Renderer/CameraProxy.h>
#include <Systems/EditorControllerSystem.h>
#include <Systems/SceneSystem.h>
#include <Systems/RenderSystem.h>
#include <Systems/UserInterface.h>
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
	gpu::ShaderLibrary& InShaderLibrary,
	gpu::Compositor& compositor
) : _Platform(InPlatform)
	, _Cursor(InCursor)
	, _Input(InInput)
	, _Screen(InScreen)
	, _Device(InDevice)
	, ShaderLibrary(InShaderLibrary)
	, _Compositor(compositor)
	, Assets(InDevice)
{
}

void Engine::Main()
{
	SceneRenderer sceneRenderer(*this);

	SystemsManager SystemsManager;

	RenderSystem RenderSystem;
	SystemsManager.Register(RenderSystem);

	EditorControllerSystem EditorControllerSystem;
	SystemsManager.Register(EditorControllerSystem);

	SceneSystem SceneSystem;
	SystemsManager.Register(SceneSystem);

	UserInterface UserInterface;
	SystemsManager.Register(UserInterface);

	SystemsManager.StartRenderSystems(*this);
	SystemsManager.StartSystems(*this);

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
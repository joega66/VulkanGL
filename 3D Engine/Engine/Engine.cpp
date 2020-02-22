#include <Renderer/SceneRenderer.h>
#include <Renderer/SceneProxy.h>
#include <Systems/EditorControllerSystem.h>
#include <Systems/GameSystem.h>
#include <Systems/TransformGizmoSystem.h>
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
	DRMDevice& InDevice, 
	DRMShaderMap& InShaderMap,
	drm::Surface& InSurface
) : _Platform(InPlatform)
	, _Cursor(InCursor)
	, _Input(InInput)
	, _Screen(InScreen)
	, Device(InDevice)
	, ShaderMap(InShaderMap)
	, Surface(InSurface)
	, Assets(InDevice)
	, Scene(*this)
	, Camera(InScreen)
{
}

void Engine::Main()
{
	SceneRenderer SceneRenderer(*this);

	SystemsManager SystemsManager;

	RenderSystem RenderSystem(*this);
	SystemsManager.Register(RenderSystem);

	EditorControllerSystem EditorControllerSystem;
	SystemsManager.Register(EditorControllerSystem);

	GameSystem GameSystem;
	SystemsManager.Register(GameSystem);

	UserInterface UserInterface(*this);
	SystemsManager.Register(UserInterface);

	SystemsManager.StartRenderSystems(*this);
	SystemsManager.StartSystems(*this);

	while (!_Platform.WindowShouldClose())
	{
		_Platform.PollEvents();

		SystemsManager.UpdateSystems(*this);

		ECS.NotifyComponentEvents();

		SystemsManager.UpdateRenderSystems(*this);

		SceneProxy SceneProxy(*this, SceneRenderer);

		SceneRenderer.Render(UserInterface, SceneProxy);

		_Cursor.Update(_Platform);

		_Input.Update(_Platform);
	}
}
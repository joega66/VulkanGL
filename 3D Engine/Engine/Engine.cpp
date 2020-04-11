#include <Renderer/SceneRenderer.h>
#include <Renderer/CameraProxy.h>
#include <Renderer/GlobalRenderData.h>
#include <Systems/EditorControllerSystem.h>
#include <Systems/SceneSystem.h>
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
	_Screen.ScreenResizeEvent([this] (int32 Width, int32 Height)
	{
		Surface.Resize(Device, Width, Height);
	});

	ECS.AddSingletonComponent<GlobalRenderData>(*this);

	SystemsManager SystemsManager;

	RenderSystem RenderSystem(*this);
	SystemsManager.Register(RenderSystem);

	EditorControllerSystem EditorControllerSystem;
	SystemsManager.Register(EditorControllerSystem);

	SceneSystem SceneSystem;
	SystemsManager.Register(SceneSystem);

	UserInterface UserInterface;
	SystemsManager.Register(UserInterface);

	SystemsManager.StartRenderSystems(*this);
	SystemsManager.StartSystems(*this);

	CameraProxy CameraProxy(*this);

	while (!_Platform.WindowShouldClose())
	{
		_Platform.PollEvents();

		ECS.NotifyComponentEvents();

		SystemsManager.UpdateSystems(*this);
		SystemsManager.UpdateRenderSystems(*this);

		CameraProxy.Update(*this);

		SceneRenderer SceneRenderer(*this);
		SceneRenderer.Render(CameraProxy);

		_Cursor.Update(_Platform);
		_Input.Update(_Platform);
	}
}
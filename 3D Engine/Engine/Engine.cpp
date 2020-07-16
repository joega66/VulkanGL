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
	gpu::Surface& InSurface
) : _Platform(InPlatform)
	, _Cursor(InCursor)
	, _Input(InInput)
	, _Screen(InScreen)
	, Device(InDevice)
	, ShaderLibrary(InShaderLibrary)
	, Surface(InSurface)
	, Assets(InDevice)
	, Camera(
		InScreen,
		glm::vec3(
			Platform::GetFloat("Engine.ini", "Camera", "LookFromX", 0.0f),
			Platform::GetFloat("Engine.ini", "Camera", "LookFromY", 0.0f),
			Platform::GetFloat("Engine.ini", "Camera", "LookFromZ", 0.0f)
		),
		glm::vec3(
			Platform::GetFloat("Engine.ini", "Camera", "LookAtX", 0.0f),
			Platform::GetFloat("Engine.ini", "Camera", "LookAtY", 0.0f),
			Platform::GetFloat("Engine.ini", "Camera", "LookAtZ", 0.0f)
		)
	)
{
}

void Engine::Main()
{
	_Screen.OnScreenResize([this] (int32 Width, int32 Height)
	{
		Surface.Resize(Device, Width, Height);
	});
	
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

	CameraProxy CameraProxy(*this);
	SceneRenderer sceneRenderer(*this);

	while (!_Platform.WindowShouldClose())
	{
		_Platform.PollEvents();

		_ECS.NotifyComponentEvents();

		SystemsManager.UpdateSystems(*this);
		SystemsManager.UpdateRenderSystems(*this);

		CameraProxy.Update(*this);

		sceneRenderer.Render(CameraProxy);

		_Cursor.Update(_Platform);
		_Input.Update(_Platform);

		Camera.SaveState();
	}
}
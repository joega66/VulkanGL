#include "CoreEngine.h"
#include <Renderer/SceneRenderer.h>
#include <Renderer/SceneProxy.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <Systems/EditorControllerSystem.h>
#include <Systems/GameSystem.h>
#include <Systems/TransformGizmoSystem.h>

class Cursor Cursor;
class Input Input;
class Screen Screen;

void CoreEngine::Run()
{
	uint8 Red[] = { 255, 0, 0, 0 };
	CMaterial::Red = drm::CreateImage(1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::ShaderResource, Red);

	uint8 Green[] = { 0, 255, 0, 0 };
	CMaterial::Green = drm::CreateImage(1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::ShaderResource, Green);

	uint8 Blue[] = { 0, 0, 255, 0 };
	CMaterial::Blue = drm::CreateImage(1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::ShaderResource, Blue);

	uint8 White[] = { 255, 255, 255, 0 };
	CMaterial::White = drm::CreateImage(1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::ShaderResource, White);

	Scene Scene;
	SceneRenderer SceneRenderer(Scene);
	SystemsManager SystemsManager;

	EditorControllerSystem EditorControllerSystem;
	SystemsManager.Register(EditorControllerSystem);

	GameSystem GameSystem;
	SystemsManager.Register(GameSystem);

	SystemsManager.StartSystems(Scene);

	while (!Platform.WindowShouldClose())
	{
		Platform.PollEvents();

		SystemsManager.UpdateSystems(Scene);
		
		SceneProxy SceneProxy(Scene);

		SceneRenderer.Render(SceneProxy);

		Cursor.Update();
		Input.Update();
	}
}
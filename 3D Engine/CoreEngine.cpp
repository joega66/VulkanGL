#include "CoreEngine.h"
#include <DRMShader.h>
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

void CoreEngine::Run(class DRM& Device, DRMShaderMap& ShaderMap)
{
	gCursor.Init();
	gInput.Init();
	gScreen.Init();

	uint8 Red[] = { 255, 0, 0, 0 };
	Material::Red = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled, Red);

	uint8 Green[] = { 0, 255, 0, 0 };
	Material::Green = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled, Green);

	uint8 Blue[] = { 0, 0, 255, 0 };
	Material::Blue = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled, Blue);

	uint8 White[] = { 255, 255, 255, 0 };
	Material::White = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled, White);

	uint8 DummyColor[] = { 234, 115, 79, 0 };
	Material::Dummy = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled, DummyColor);

	Scene Scene(Device, ShaderMap);
	SceneRenderer SceneRenderer(Device, Scene);
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
		
		SceneProxy SceneProxy(Device, Scene);

		SceneRenderer.Render(SceneProxy);

		gCursor.Update();
		gInput.Update();
	}
}
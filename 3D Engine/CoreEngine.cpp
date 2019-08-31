#include "CoreEngine.h"
#include <Renderer/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <ECS/EditorControllerSystem.h>
#include <ECS/GameSystem.h>
#include <ECS/LightSystem.h>
#include <ECS/StaticMeshSystem.h>
#include <ECS/TransformGizmoSystem.h>

class Cursor Cursor;
class Input Input;
class Screen Screen;

void CoreEngine::Run()
{
	uint8 Red[] = { 255, 0, 0, 0 };
	CMaterial::Red = drm::CreateImage(1, 1, EImageFormat::R8G8B8A8_UNORM, EResourceUsage::ShaderResource, Red);

	uint8 Green[] = { 0, 255, 0, 0 };
	CMaterial::Green = drm::CreateImage(1, 1, EImageFormat::R8G8B8A8_UNORM, EResourceUsage::ShaderResource, Green);

	uint8 Blue[] = { 0, 0, 255, 0 };
	CMaterial::Blue = drm::CreateImage(1, 1, EImageFormat::R8G8B8A8_UNORM, EResourceUsage::ShaderResource, Blue);

	std::array<std::string, 6> Cubemap = 
	{
		"../Images/Cubemaps/DownUnder/down-under_rt.tga", "../Images/Cubemaps/DownUnder/down-under_lf.tga",
		"../Images/Cubemaps/DownUnder/down-under_up.tga", "../Images/Cubemaps/DownUnder/down-under_dn.tga",
		"../Images/Cubemaps/DownUnder/down-under_bk.tga", "../Images/Cubemaps/DownUnder/down-under_ft.tga",
	};

	// Engine primitives/default assets
	GAssetManager.LoadCubemap("Engine-Cubemap-Default", Cubemap);
	GAssetManager.LoadImage("Engine-Diffuse-Default", "../Images/Frozen-Ice-Texture.jpg");
	GAssetManager.LoadStaticMesh("Transform-Gizmo", "../Meshes/Primitives/TransformGizmo/TransformGizmo.obj");
	GAssetManager.LoadStaticMesh("Cube", "../Meshes/Primitives/Cube.obj");

	EditorControllerSystem EditorControllerSystem;
	SystemsHerder.Register(EditorControllerSystem);

	GameSystem GameSystem;
	SystemsHerder.Register(GameSystem);

	LightSystem LightSystem;
	SystemsHerder.Register(LightSystem);

	StaticMeshSystem StaticMeshSystem;
	SystemsHerder.Register(StaticMeshSystem);

	TransformGizmoSystem TransformGizmoSystem;
	SystemsHerder.Register(TransformGizmoSystem);

	auto& Scene = Scene::Get();

	SystemsHerder.StartSystems();

	while (!Platform.WindowShouldClose())
	{
		Platform.PollEvents();
		SystemsHerder.UpdateSystems();
		Scene.Render();
		Platform.Finish();
	}
}
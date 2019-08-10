#include "CoreEngine.h"
#include <Renderer/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <ComponentSystems/StaticMeshSystem.h>
#include <ComponentSystems/TransformGizmoSystem.h>
#include <ComponentSystems/EditorControllerSystem.h>
#include <ComponentSystems/GameSystem.h>
#include <ComponentSystems/LightSystem.h>

class Cursor Cursor;
class Input Input;
// @todo Move this to View?
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

	// @todo Code generation. Or, for now, COMPONENT_SYSTEM() macro.

	EditorControllerSystem EditorControllerSystem;
	ComponentSystemManager.AddComponentSystem(EditorControllerSystem);
	
	StaticMeshSystem StaticMeshSystem;
	ComponentSystemManager.AddComponentSystem(StaticMeshSystem);

	TransformGizmoSystem TransformGizmoSystem;
	ComponentSystemManager.AddComponentSystem(TransformGizmoSystem);

	LightSystem LightSystem;
	ComponentSystemManager.AddComponentSystem(LightSystem);

	GameSystem GameSystem;
	ComponentSystemManager.AddComponentSystem(GameSystem);

	auto& Scene = Scene::Get();

	// @todo ViewportChanged lambda callbacks
	while (!GPlatform->WindowShouldClose())
	{
		GPlatform->PollEvents();
		ComponentSystemManager.Update();
		Scene.Render();
		GPlatform->EndFrame();
	}
}
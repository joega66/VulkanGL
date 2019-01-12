#include "CoreEngine.h"
#include <Renderer/Scene.h>
#include <Engine/AssetManager.h>
#include <ComponentSystems/StaticMeshSystem.h>
#include <ComponentSystems/TransformGizmoSystem.h>
#include <ComponentSystems/EditorControllerSystem.h>
#include <ComponentSystems/GameSystem.h>
#include <Components/CTransform.h>
#include <Components/CTransform.h>

class Cursor Cursor;
class Input Input;

void CoreEngine::Run()
{
	std::array<std::string, 6> Cubemap = {
		"../Images/Cubemaps/DownUnder/down-under_rt.tga", "../Images/Cubemaps/DownUnder/down-under_lf.tga",
		"../Images/Cubemaps/DownUnder/down-under_up.tga", "../Images/Cubemaps/DownUnder/down-under_dn.tga",
		"../Images/Cubemaps/DownUnder/down-under_bk.tga", "../Images/Cubemaps/DownUnder/down-under_ft.tga",
	};

	// Engine primitives/default assets
	GAssetManager.LoadCubemap("Engine-Cubemap-Default", Cubemap);
	GAssetManager.LoadImage("Engine-Diffuse-Default", "../Images/Frozen-Ice-Texture.jpg");
	GAssetManager.LoadStaticMesh("Transform-Gizmo", "../Meshes/Primitives/TransformGizmo/TransformGizmo.obj");
	GAssetManager.LoadStaticMesh("Cube", "../Meshes/Primitives/Cube.obj");

	auto& Scene = Scene::Get();

	// @todo Code generation. Or, for now, COMPONENT_SYSTEM() macro. 

	EditorControllerSystem EditorControllerSystem;
	GComponentSystemManager.AddComponentSystem(EditorControllerSystem);

	StaticMeshSystem StaticMeshSystem;
	GComponentSystemManager.AddComponentSystem(StaticMeshSystem);

	TransformGizmoSystem TransformGizmoSystem;
	GComponentSystemManager.AddComponentSystem(TransformGizmoSystem);

	GameSystem GameSystem;
	GComponentSystemManager.AddComponentSystem(GameSystem);

	while (!GPlatform->WindowShouldClose())
	{
		GPlatform->PollEvents();
		GComponentSystemManager.UpdateSystems();
		GLBeginRender();
		Scene.Render();
		GLEndRender();
		GPlatform->EndFrame();
	}
}
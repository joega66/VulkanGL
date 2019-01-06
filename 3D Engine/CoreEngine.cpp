#include "CoreEngine.h"
#include <Renderer/Scene.h>
#include <Engine/ResourceManager.h>
#include <ComponentSystems/StaticMeshSystem.h>
#include <ComponentSystems/TransformGizmoSystem.h>
#include <ComponentSystems/EditorControllerSystem.h>
#include <ComponentSystems/GameSystem.h>

void CoreEngine::Run()
{
	GAssetManager.LoadImage("Engine-Diffuse-Default", "../Images/Frozen-Ice-Texture.jpg");

	std::array<std::string, 6> Cubemap = { 
		"../Images/Cubemaps/DownUnder/down-under_rt.tga", "../Images/Cubemaps/DownUnder/down-under_lf.tga",
		"../Images/Cubemaps/DownUnder/down-under_ft.tga", "../Images/Cubemaps/DownUnder/down-under_bk.tga"
		"../Images/Cubemaps/DownUnder/down-under_up.tga", "../Images/Cubemaps/DownUnder/down-under_dn.tga",
	};
	GAssetManager.LoadCubemap("Engine-Cubemap-Default", Cubemap);

	GAssetManager.LoadStaticMesh("Cube", "../Meshes/Primitives/Cube.obj");
	GAssetManager.LoadStaticMesh("Ivysaur", "../Meshes/Ivysaur/Pokemon.obj");

	auto& Scene = Scene::Get();

	// @todo Code generation

	StaticMeshSystem StaticMeshSystem;
	GComponentSystemManager.AddRenderSystem(StaticMeshSystem);

	TransformGizmoSystem TransformGizmoSystem;
	GComponentSystemManager.AddComponentSystem(TransformGizmoSystem);

	EditorControllerSystem EditorControllerSystem;
	GComponentSystemManager.AddComponentSystem(EditorControllerSystem);

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
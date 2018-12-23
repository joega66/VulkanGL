#include "CoreEngine.h"
#include "Renderer/Scene.h"
#include "Engine/ResourceManager.h"
#include "ComponentSystems/StaticMeshSystem.h"
#include "ComponentSystems/TransformGizmoSystem.h"
#include "ComponentSystems/EditorControllerSystem.h"

void CoreEngine::Run()
{
	GAssetManager.SaveImage("Engine-Diffuse-Default", "../Images/Frozen-Ice-Texture.jpg");

	StaticMeshRef Cube = GAssetManager.SaveStaticMesh("Cube", "../Meshes/Primitives/Cube.obj");
	StaticMeshRef StaticMesh = GAssetManager.SaveStaticMesh("Ivysaur", "../Meshes/Ivysaur/Pokemon.obj");
	
	Scene Scene;

	auto Entity = GEntityManager.CreateEntity();
	Entity.AddComponent<CStaticMesh>(StaticMesh);

	StaticMeshSystem StaticMeshSystem;
	GComponentSystemManager.AddRenderSystem(StaticMeshSystem);

	TransformGizmoSystem TransformGizmoSystem;
	GComponentSystemManager.AddComponentSystem(TransformGizmoSystem);

	EditorControllerSystem EditorControllerSystem;
	GComponentSystemManager.AddComponentSystem(EditorControllerSystem);

	while (!GPlatform->WindowShouldClose())
	{
		GPlatform->PollEvents();

		GComponentSystemManager.UpdateSystems(Scene);

		GLBeginRender();

		Scene.Render();

		GLEndRender();

		GPlatform->EndFrame();
	}
}
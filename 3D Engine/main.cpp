#include "Platform/WindowsPlatform.h"
#include "Vulkan/VulkanGL.h"
#include "Renderer/Scene.h"
#include "Engine/ResourceManager.h"
#include "Engine/EditorController.h"
#include "Components/CStaticMesh.h"
#include "Components/Entity.h"
#include "ComponentSystems/StaticMeshSystem.h"
#include "ComponentSystems/TransformGizmoSystem.h"
#include <cxxopts.hpp>

void RunEngine()
{
	GAssetManager->SaveImage("Engine-Diffuse-Default", "../Images/Frozen-Ice-Texture.jpg");
	
	StaticMeshRef Cube = GAssetManager->SaveStaticMesh("Cube", "../Meshes/Primitives/Cube.obj");
	StaticMeshRef StaticMesh = GAssetManager->SaveStaticMesh("Ivysaur", "../Meshes/Ivysaur/Pokemon.obj");

	// @todo Make this a component system?
	EditorController EditorController;
	Scene Scene;

	// @todo We can return an entity by value, since it's just an integer handle!
	auto& Entity = GEntityManager.CreateEntity();
	Entity.AddComponent<CStaticMesh>(StaticMesh);
	auto& Transform = Entity.AddComponent<CTransform>();

	StaticMeshSystem StaticMeshSystem;
	GComponentSystemManager.AddRenderSystem(&StaticMeshSystem);

	TransformGizmoSystem TransformGizmoSystem;
	GComponentSystemManager.AddComponentSystem(&TransformGizmoSystem);
	
	while (!GPlatform->WindowShouldClose())
	{
		GPlatform->PollEvents();

		EditorController.Update(Scene.View);

		GComponentSystemManager.UpdateSystems(&Scene);

		GLBeginRender();

		Scene.Render();

		GLEndRender();

		GPlatform->EndFrame();
	}
}

int main(int argc, char* argv[])
{
	cxxopts::Options Options("VulkanGL", "A Vulkan-based framework for graphics demos :)");

	Options.add_options()
		("vulkan", "Enable Vulkan graphics library")
		("w,width", "Window width", cxxopts::value<int32>())
		("h,height", "Window height", cxxopts::value<int32>());

	cxxopts::ParseResult Result = Options.parse(argc, argv);

	int32 WinX = Result["width"].as<int32>();
	int32 WinY = Result["height"].as<int32>();

#ifdef _WIN32
	GPlatform = MakeRef<WindowsPlatform>();
#endif

	GPlatform->OpenWindow(WinX, WinY);

	if (Result.count("vulkan"))
	{
		GRender = MakeRef<VulkanGL>();
		GShaderCompiler = MakeRef<VulkanShaderCompiler>();
	}

	GAssetManager = MakeRef<AssetManager>();

	GRender->InitGL();

	RunEngine();

	GRender->ReleaseGL();

	return 0;
}
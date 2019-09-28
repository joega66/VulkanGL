#include "GameSystem.h"
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Components/CLight.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>

void GameSystem::Start(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	// Create the cube prefab.
	auto Cube = ECS.CreatePrefab("Cube");
	ECS.AddComponent<CStaticMesh>(Cube, Scene.Assets.GetStaticMesh("Cube"));

	{
		// Create the Sponza entity.

		auto Sponza = ECS.CreateEntity();

		auto SponzaMesh = Scene.Assets.LoadStaticMesh("Sponza", "../Meshes/Sponza/sponza.obj");

		ECS.AddComponent<CStaticMesh>(Sponza, SponzaMesh);
	}

	{
		// Create the Directional Light entity.

		auto Light = ECS.CreateFromPrefab("Cube");

		auto& DirectionalLight = ECS.AddComponent<CDirectionalLight>(Light);
		DirectionalLight.Intensity = 10.0f;
		DirectionalLight.Direction = glm::vec3(1.0f);

		auto& Material = ECS.AddComponent<CMaterial>(Light);
		Material.Diffuse = CMaterial::White;

		auto& Transform = ECS.GetComponent<CTransform>(Light);
		Transform.Scale(glm::vec3(0.1f));
		Transform.Translate(glm::vec3(1.0f));
	}
}

void GameSystem::Update(Scene& Scene)
{
}
#include "GameSystem.h"
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Engine/Engine.h>
#include <Engine/AssetManager.h>

void GameSystem::Start(Engine& Engine)
{
	auto& ECS = Engine.ECS;

	{
		// Create the Sponza entities.
		std::vector<const StaticMesh*> SponzaMeshes = Engine.Assets.LoadStaticMesh("Sponza", "../Meshes/Sponza/Sponza.gltf", true);
		for (auto StaticMesh : SponzaMeshes)
		{
			auto SponzaEntity = ECS.CreateEntity();
			ECS.AddComponent(SponzaEntity, StaticMeshComponent(StaticMesh));
			ECS.AddComponent(SponzaEntity, Material(StaticMesh->Materials.front()));

			auto& Transform = ECS.GetComponent<class Transform>(SponzaEntity);
			Transform.Scale(ECS, glm::vec3(0.1f));
		}
	}

	{
		std::vector<const StaticMesh*> HelmetMesh = Engine.Assets.LoadStaticMesh("DamagedHelmet", "../Meshes/DamagedHelmet/DamagedHelmet.gltf");

		auto HelmetEntity = ECS.CreateEntity();
		ECS.AddComponent(HelmetEntity, StaticMeshComponent(HelmetMesh.front()));
		ECS.AddComponent(HelmetEntity, Material(HelmetMesh.front()->Materials.front()));

		auto& Transform = ECS.GetComponent<class Transform>(HelmetEntity);
		Transform.Scale(ECS, glm::vec3(2));
		Transform.Translate(ECS, glm::vec3(0.0, 5, -10));
		Transform.Rotate(ECS, glm::vec3(1, 0, 0), 90);
	}
	
	{
		// Create the Directional Light entity.
		const float64 X = Platform::GetFloat64("Engine.ini", "DirectionalLight", "X", 1.0f);
		const float64 Y = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Y", 1.0f);
		const float64 Z = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Z", 1.0f);

		auto Light = ECS.CreateEntity();
		auto& DirectionalLight = ECS.AddComponent(Light, struct DirectionalLight());
		DirectionalLight.Intensity = 10.0f;
		DirectionalLight.Direction = glm::vec3(X, Y, Z);
		DirectionalLight.ShadowType = EShadowType::Soft;
		DirectionalLight.DepthBiasConstantFactor = 1.75f;
		DirectionalLight.DepthBiasSlopeFactor = 1.75f;
	}
}

void GameSystem::Update(Engine& Engine)
{
	for (auto& Entity : Engine.ECS.GetEntities<DirectionalLight>())
	{
		auto& DirectionalLight = Engine.ECS.GetComponent<struct DirectionalLight>(Entity);
		const float64 X = Platform::GetFloat64("Engine.ini", "DirectionalLight", "X", 1.0f);
		const float64 Y = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Y", 1.0f);
		const float64 Z = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Z", 1.0f);
		DirectionalLight.Direction = glm::vec3(X, Y, Z);
	}
}
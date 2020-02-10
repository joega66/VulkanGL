#include "GameSystem.h"
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>

void GameSystem::Start(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	{
		// Create the Sponza entities.
		std::vector<const StaticMesh*> SponzaMeshes = Scene.Assets.LoadStaticMesh("Sponza", "../Meshes/Sponza/sponza.obj", true);
		std::for_each(SponzaMeshes.begin(), SponzaMeshes.end(), [&] (const StaticMesh* StaticMesh)
		{
			auto SponzaEntity = ECS.CreateEntity();
			ECS.AddComponent<StaticMeshComponent>(SponzaEntity, StaticMesh);
			ECS.AddComponent<Material>(SponzaEntity, StaticMesh->Materials.front());

			auto& Transform = ECS.GetComponent<class Transform>(SponzaEntity);
			Transform.Scale(glm::vec3(0.1f));
		});
	}

	const auto& Helmet = Scene.Assets.LoadStaticMesh("DamagedHelmet", "../Meshes/DamagedHelmet/DamagedHelmet.gltf");
	auto HelmetEntity = ECS.CreateEntity();
	auto& Transform = ECS.GetComponent<class Transform>(HelmetEntity);
	Transform.Scale(glm::vec3(2));
	Transform.Translate(glm::vec3(0.0, 5, -10));
	Transform.Rotate(glm::vec3(1, 0, 0), 90);
	ECS.AddComponent<StaticMeshComponent>(HelmetEntity, Helmet[0]);
	ECS.AddComponent<Material>(HelmetEntity, Helmet.front()->Materials.front());

	{
		// Create the Directional Light entity.
		const float64 X = Platform::GetFloat64("Engine.ini", "DirectionalLight", "X", 1.0f);
		const float64 Y = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Y", 1.0f);
		const float64 Z = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Z", 1.0f);

		auto Light = ECS.CreateEntity();
		auto& DirectionalLight = ECS.AddComponent<struct DirectionalLight>(Light);
		DirectionalLight.Intensity = 10.0f;
		DirectionalLight.Direction = glm::vec3(X, Y, Z);
		DirectionalLight.ShadowType = EShadowType::Soft;
		DirectionalLight.DepthBiasConstantFactor = 1.75f;
		DirectionalLight.DepthBiasSlopeFactor = 1.75f;
	}
}

void GameSystem::Update(Scene& Scene)
{
	for (auto& Entity : Scene.ECS.GetEntities<DirectionalLight>())
	{
		auto& DirectionalLight = Scene.ECS.GetComponent<struct DirectionalLight>(Entity);
		const float64 X = Platform::GetFloat64("Engine.ini", "DirectionalLight", "X", 1.0f);
		const float64 Y = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Y", 1.0f);
		const float64 Z = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Z", 1.0f);
		DirectionalLight.Direction = glm::vec3(X, Y, Z);
	}
}
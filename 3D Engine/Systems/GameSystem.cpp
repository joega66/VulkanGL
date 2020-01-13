#include "GameSystem.h"
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/CLight.h>
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

	{
		// Create the Directional Light entity.
		const float64 X = Platform::GetFloat64("Engine.ini", "DirectionalLight", "X", 1.0f);
		const float64 Y = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Y", 1.0f);
		const float64 Z = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Z", 1.0f);

		auto Light = ECS.CreateEntity();
		auto& DirectionalLight = ECS.AddComponent<CDirectionalLight>(Light);
		DirectionalLight.Intensity = 10.0f;
		DirectionalLight.Direction = glm::vec3(X, Y, Z);
		DirectionalLight.ShadowType = EShadowType::Soft;
		DirectionalLight.DepthBiasConstantFactor = 1.75f;
		DirectionalLight.DepthBiasSlopeFactor = 1.75f;
	}
}

void GameSystem::Update(Scene& Scene)
{
}
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
		auto& Transform = ECS.GetComponent<CTransform>(Sponza);
		Transform.Scale(glm::vec3(0.1f));
	}

	{
		// Create the Directional Light entity.
		const float64 X = Platform.GetFloat64("Engine.ini", "DirectionalLight", "X", 1.0f);
		const float64 Y = Platform.GetFloat64("Engine.ini", "DirectionalLight", "Y", 1.0f);
		const float64 Z = Platform.GetFloat64("Engine.ini", "DirectionalLight", "Z", 1.0f);

		auto Light = ECS.CreateFromPrefab("Cube");
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
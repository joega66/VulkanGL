#include "GameSystem.h"
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Components/CLight.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>

void GameSystem::Start(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	auto Sponza = ECS.CreateEntity();
	auto SponzaMesh = Scene.Assets.LoadStaticMesh("Sponza", "../Meshes/Sponza/sponza.obj");
	ECS.AddComponent<CStaticMesh>(Sponza, SponzaMesh);

	auto Cube = ECS.CreatePrefab("Cube");
	ECS.AddComponent<CStaticMesh>(Cube, Scene.Assets.GetStaticMesh("Cube"));

	auto LightEntity = ECS.CreateFromPrefab("Cube");
	auto& Light = ECS.AddComponent<CLight>(LightEntity);
	auto& Material = ECS.AddComponent<CMaterial>(LightEntity);
	Material.Diffuse = CMaterial::Blue;

	auto& LightTransform = ECS.GetComponent<CTransform>(LightEntity);
	LightTransform.Scale(glm::vec3(0.1f));
	LightTransform.Translate(glm::vec3(1.0f));
}

void GameSystem::Update(Scene& Scene)
{
}
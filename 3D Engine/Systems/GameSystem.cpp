#include "GameSystem.h"
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Components/CLight.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>

void GameSystem::Start(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	GAssetManager.LoadStaticMesh("Ivysaur", "../Meshes/Ivysaur/Pokemon.obj");

	Entity Cube = ECS.CreatePrefab("Cube");
	ECS.AddComponent<CStaticMesh>(Cube, GAssetManager.GetStaticMesh("Cube"));

	auto Cube1 = ECS.CreateFromPrefab("Cube");
	auto Cube2 = ECS.CreateFromPrefab("Cube");
	auto Cube3 = ECS.CreateFromPrefab("Cube");
	auto Cube4 = ECS.CreateFromPrefab("Cube");

	auto& Transform1 = ECS.GetComponent<CTransform>(Cube1);
	Transform1.Translate(glm::vec3(-10.0f, 0.0f, 0.0f));

	auto& Transform2 = ECS.GetComponent<CTransform>(Cube2);
	Transform2.Translate(glm::vec3(10.0f, 0.0f, 0.0f));

	auto& Transform3 = ECS.GetComponent<CTransform>(Cube3);
	Transform3.Translate(glm::vec3(0.0f, 0.0f, -10.0f));

	auto& Transform4 = ECS.GetComponent<CTransform>(Cube4);
	Transform4.Translate(glm::vec3(0.0f, 0.0f, 10.0f));

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
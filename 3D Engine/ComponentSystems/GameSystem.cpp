#include "GameSystem.h"
#include <Components/Entity.h>
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Components/CLight.h>
#include <Engine/AssetManager.h>

GameSystem::GameSystem()
{
	GAssetManager.LoadStaticMesh("Ivysaur", "../Meshes/Ivysaur/Pokemon.obj");

	Entity Cube = GEntityManager.CreatePrefab("Cube");
	Cube.AddComponent<CStaticMesh>(GAssetManager.GetStaticMesh("Cube"));

	auto Cube1 = GEntityManager.CreateFromPrefab("Cube");
	auto Cube2 = GEntityManager.CreateFromPrefab("Cube");
	auto Cube3 = GEntityManager.CreateFromPrefab("Cube");
	auto Cube4 = GEntityManager.CreateFromPrefab("Cube");

	auto& Transform1 = Cube1.GetComponent<CTransform>();
	Transform1.Translate(glm::vec3(-10.0f, 0.0f, 0.0f));

	auto& Transform2 = Cube2.GetComponent<CTransform>();
	Transform2.Translate(glm::vec3(10.0f, 0.0f, 0.0f));

	auto& Transform3 = Cube3.GetComponent<CTransform>();
	Transform3.Translate(glm::vec3(0.0f, 0.0f, -10.0f));

	auto& Transform4 = Cube4.GetComponent<CTransform>();
	Transform4.Translate(glm::vec3(0.0f, 0.0f, 10.0f));

	/*auto Ivysaur = GEntityManager.CreateEntity();
	Ivysaur.AddComponent<CStaticMesh>(GAssetManager.GetStaticMesh("Ivysaur"));

	auto& IvysaurTransform = Ivysaur.GetComponent<CTransform>();
	IvysaurTransform.Translate(glm::vec3(10.0f, 0.0f, 10.0f));*/

	auto LightEntity = GEntityManager.CreateFromPrefab("Cube");
	auto& Light = LightEntity.AddComponent<CLight>();

	auto& LightTransform = LightEntity.GetComponent<CTransform>();
	LightTransform.Scale(glm::vec3(0.1f));
	LightTransform.Translate(glm::vec3(1.0f));

	//auto& Material = LightEntity.AddComponent<CMaterial>();
	//Material.Diffuse = glm::vec4(Light.Color, 1.0f);
}

void GameSystem::Update()
{
}
#include "GameSystem.h"
#include <ECS/EntityManager.h>
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Components/CLight.h>
#include <Engine/AssetManager.h>

void GameSystem::Start()
{
	GAssetManager.LoadStaticMesh("Ivysaur", "../Meshes/Ivysaur/Pokemon.obj");

	Entity Cube = GEntityManager.CreatePrefab("Cube");
	GEntityManager.AddComponent<CStaticMesh>(Cube, GAssetManager.GetStaticMesh("Cube"));

	auto Cube1 = GEntityManager.CreateFromPrefab("Cube");
	auto Cube2 = GEntityManager.CreateFromPrefab("Cube");
	auto Cube3 = GEntityManager.CreateFromPrefab("Cube");
	auto Cube4 = GEntityManager.CreateFromPrefab("Cube");

	auto& Transform1 = GEntityManager.GetComponent<CTransform>(Cube1);
	Transform1.Translate(glm::vec3(-10.0f, 0.0f, 0.0f));

	auto& Transform2 = GEntityManager.GetComponent<CTransform>(Cube2);
	Transform2.Translate(glm::vec3(10.0f, 0.0f, 0.0f));

	auto& Transform3 = GEntityManager.GetComponent<CTransform>(Cube3);
	Transform3.Translate(glm::vec3(0.0f, 0.0f, -10.0f));

	auto& Transform4 = GEntityManager.GetComponent<CTransform>(Cube4);
	Transform4.Translate(glm::vec3(0.0f, 0.0f, 10.0f));

	/*auto Ivysaur = GEntityManager.CreateEntity();
	Ivysaur.AddComponent<CStaticMesh>(GAssetManager.GetStaticMesh("Ivysaur"));

	auto& IvysaurTransform = Ivysaur.GetComponent<CTransform>();
	IvysaurTransform.Translate(glm::vec3(10.0f, 0.0f, 10.0f));*/

	auto LightEntity = GEntityManager.CreateFromPrefab("Cube");
	auto& Light = GEntityManager.AddComponent<CLight>(LightEntity);
	auto& Material = GEntityManager.AddComponent<CMaterial>(LightEntity);
	Material.Diffuse = CMaterial::Blue;

	auto& LightTransform = GEntityManager.GetComponent<CTransform>(LightEntity);
	LightTransform.Scale(glm::vec3(0.1f));
	LightTransform.Translate(glm::vec3(1.0f));
}

void GameSystem::Update(Scene& Scene)
{
}
#include "GameSystem.h"
#include <Components/Entity.h>
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Engine/ResourceManager.h>

GameSystem::GameSystem()
{
	Entity Cube = GEntityManager.CreatePrefab("Cube");
	Cube.AddComponent<CStaticMesh>(GAssetManager.GetStaticMesh("Cube"));

	// @test
	auto E1 = GEntityManager.CreateFromPrefab("Cube");
	auto E2 = GEntityManager.CreateFromPrefab("Cube");
	auto E3 = GEntityManager.CreateFromPrefab("Cube");
	auto E4 = GEntityManager.CreateFromPrefab("Cube");

	auto& Transform = E1.GetComponent<CTransform>();
	Transform.Translate(glm::vec3(-10.0f, 0.0f, 0.0f));

	auto& Transform1 = E2.GetComponent<CTransform>();
	Transform1.Translate(glm::vec3(10.0f, 0.0f, 0.0f));

	auto& Transform2 = E3.GetComponent<CTransform>();
	Transform2.Translate(glm::vec3(0.0f, 0.0f, -10.0f));

	auto& Transform3 = E4.GetComponent<CTransform>();
	Transform3.Translate(glm::vec3(0.0f, 0.0f, 10.0f));

	auto Entity = GEntityManager.CreateEntity();
	Entity.AddComponent<CStaticMesh>(GAssetManager.GetStaticMesh("Ivysaur"));
}

void GameSystem::Update()
{
}
#include "SceneSystem.h"
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Engine/Engine.h>
#include <Engine/AssetManager.h>

void SceneSystem::Start(Engine& Engine)
{
	auto& ECS = Engine.ECS;

	const std::string ScenePath = Platform::GetString("Engine.ini", "Scene", "Path", "../Meshes/Sponza/Sponza.gltf");
	const SceneLoadRequest SceneLoadRequest = { ScenePath, false };

	HandleSceneLoadRequest(Engine, SceneLoadRequest);

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

void SceneSystem::Update(Engine& Engine)
{
	auto& ECS = Engine.ECS;

	for (auto& Entity : ECS.GetEntities<DirectionalLight>())
	{
		auto& DirectionalLight = ECS.GetComponent<struct DirectionalLight>(Entity);
		const float64 X = Platform::GetFloat64("Engine.ini", "DirectionalLight", "X", 1.0f);
		const float64 Y = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Y", 1.0f);
		const float64 Z = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Z", 1.0f);
		DirectionalLight.Direction = glm::vec3(X, Y, Z);
	}

	for (auto& Entity : ECS.GetEntities<struct SceneLoadRequest>())
	{
		auto& SceneLoadRequest = ECS.GetComponent<struct SceneLoadRequest>(Entity);
		HandleSceneLoadRequest(Engine, SceneLoadRequest);
		ECS.Destroy(Entity);
	}
}

void SceneSystem::HandleSceneLoadRequest(Engine& Engine, const SceneLoadRequest& SceneLoadRequest)
{
	auto& ECS = Engine.ECS;

	if (SceneLoadRequest.DestroyOldEntities)
	{
		for (auto& Entity : ECS.GetEntities<StaticMeshComponent>())
		{
			ECS.Destroy(Entity);
		}
	}

	const std::vector<const StaticMesh*> Scene = Engine.Assets.LoadStaticMesh(SceneLoadRequest.Path, true);
	
	for (auto StaticMesh : Scene)
	{
		auto Entity = ECS.CreateEntity();
		ECS.AddComponent(Entity, StaticMeshComponent(StaticMesh, StaticMesh->Materials.front()));

		auto& Transform = ECS.GetComponent<class Transform>(Entity);
		Transform.Scale(ECS, glm::vec3(0.1f));
	}
}

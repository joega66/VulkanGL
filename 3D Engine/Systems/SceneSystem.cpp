#include "SceneSystem.h"
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/Bounds.h>
#include <Components/SkyboxComponent.h>
#include <Engine/Engine.h>
#include <Engine/AssetManager.h>

void SceneSystem::Start(Engine& Engine)
{
	auto& ECS = Engine.ECS;

	const std::string ScenePath = Platform::GetString("Engine.ini", "Scene", "Path", "../Assets/Meshes/Sponza/Sponza.gltf");
	const SceneLoadRequest SceneLoadReq(ScenePath, false);

	HandleSceneLoadRequest(Engine, SceneLoadReq);

	const float64 X = Platform::GetFloat64("Engine.ini", "DirectionalLight", "X", 1.0f);
	const float64 Y = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Y", 1.0f);
	const float64 Z = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Z", 1.0f);

	auto Light = ECS.CreateEntity("DirectionalLight");
	auto& DirectionalLight = ECS.AddComponent(Light, struct DirectionalLight());
	DirectionalLight.Intensity = 10.0f;
	DirectionalLight.Direction = glm::vec3(X, Y, Z);
	DirectionalLight.ShadowType = EShadowType::Soft;
	DirectionalLight.DepthBiasConstantFactor = 1.75f;
	DirectionalLight.DepthBiasSlopeFactor = 1.75f;

	const std::string SkyboxPath = Platform::GetString("Engine.ini", "Scene", "Skybox", "../Assets/Cube_Maps/White_Cliff_Top/");
	Skybox* Skybox = Engine.Assets.LoadSkybox("Default_Skybox", SkyboxPath);

	auto SkyboxEntity = ECS.CreateEntity("Skybox");
	ECS.AddComponent(SkyboxEntity, SkyboxComponent(Skybox));
}

void SceneSystem::Update(Engine& Engine)
{
	auto& ECS = Engine.ECS;

	for (auto& Entity : ECS.GetEntities<SceneLoadRequest>())
	{
		auto& SceneLoadReq = ECS.GetComponent<SceneLoadRequest>(Entity);
		HandleSceneLoadRequest(Engine, SceneLoadReq);
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
		auto Entity = ECS.CreateEntity(StaticMesh->Name.c_str());
		ECS.AddComponent(Entity, StaticMeshComponent(StaticMesh, StaticMesh->Materials.front()));

		auto& Transform = ECS.GetComponent<class Transform>(Entity);
		Transform.Scale(ECS, glm::vec3(0.1f));
	}
}

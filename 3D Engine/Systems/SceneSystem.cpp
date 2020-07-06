#include "SceneSystem.h"
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/Bounds.h>
#include <Components/SkyboxComponent.h>
#include <Engine/Engine.h>
#include <Engine/AssetManager.h>

void SceneSystem::Start(Engine& engine)
{
	auto& ecs = engine._ECS;

	const std::string scenePath = Platform::GetString("Engine.ini", "Scene", "Path", "../Assets/Meshes/Sponza/Sponza.gltf");
	const SceneLoadRequest sceneLoadReq(scenePath, false);

	HandleSceneLoadRequest(engine, sceneLoadReq);

	const float64 x = Platform::GetFloat64("Engine.ini", "DirectionalLight", "X", 1.0f);
	const float64 y = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Y", 1.0f);
	const float64 z = Platform::GetFloat64("Engine.ini", "DirectionalLight", "Z", 1.0f);

	auto light = ecs.CreateEntity("DirectionalLight");
	auto& directionalLight = ecs.AddComponent(light, DirectionalLight());
	directionalLight.Intensity = Platform::GetFloat("Engine.ini", "DirectionalLight", "Intensity", 5.0f);
	directionalLight.Direction = glm::vec3(x, y, z);
	directionalLight.ShadowType = EShadowType::Soft;
	directionalLight.DepthBiasConstantFactor = 1.75f;
	directionalLight.DepthBiasSlopeFactor = 1.75f;

	const std::string skyboxPath = Platform::GetString("Engine.ini", "Scene", "Skybox", "../Assets/Cube_Maps/White_Cliff_Top/");
	Skybox* skybox = engine.Assets.LoadSkybox("Default_Skybox", skyboxPath);

	auto skyboxEntity = ecs.CreateEntity("Skybox");
	ecs.AddComponent(skyboxEntity, SkyboxComponent(skybox));
}

void SceneSystem::Update(Engine& engine)
{
	auto& ecs = engine._ECS;

	for (auto& entity : ecs.GetEntities<SceneLoadRequest>())
	{
		auto& sceneLoadReq = ecs.GetComponent<SceneLoadRequest>(entity);
		HandleSceneLoadRequest(engine, sceneLoadReq);
		ecs.Destroy(entity);
	}
}

void SceneSystem::HandleSceneLoadRequest(Engine& engine, const SceneLoadRequest& sceneLoadRequest)
{
	auto& ecs = engine._ECS;

	if (sceneLoadRequest.destroyOldEntities)
	{
		for (auto& entity : ecs.GetEntities<StaticMeshComponent>())
		{
			ecs.Destroy(entity);
		}
	}

	const std::vector<const StaticMesh*> scene = engine.Assets.LoadStaticMesh(sceneLoadRequest.path, true);
	const float scale = Platform::GetFloat("Engine.ini", "Scene", "Scale", 0.1f);

	for (auto staticMesh : scene)
	{
		auto entity = ecs.CreateEntity(staticMesh->Name.c_str());
		ecs.AddComponent(entity, StaticMeshComponent(staticMesh, staticMesh->Materials.front()));

		auto& transform = ecs.GetComponent<Transform>(entity);
		transform.Scale(ecs, glm::vec3(scale));
	}
}

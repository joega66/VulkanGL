#include "SceneSystem.h"
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/SkyboxComponent.h>
#include <Engine/Engine.h>
#include <Engine/AssetManager.h>

void SceneSystem::Start(Engine& engine)
{
	auto& ecs = engine._ECS;

	// Create the camera.
	auto cameraEntity = ecs.CreateEntity("Camera");
	ecs.AddComponent(cameraEntity, Camera(
		engine._Screen,
		glm::vec3(Platform::GetFloat("Engine.ini", "Camera", "LookFromX", 0.0f),
				  Platform::GetFloat("Engine.ini", "Camera", "LookFromY", 0.0f),
				  Platform::GetFloat("Engine.ini", "Camera", "LookFromZ", 0.0f)),
		glm::vec3(Platform::GetFloat("Engine.ini", "Camera", "LookAtX", 0.0f),
				  Platform::GetFloat("Engine.ini", "Camera", "LookAtY", 0.0f),
				  Platform::GetFloat("Engine.ini", "Camera", "LookAtZ", 0.0f))));

	// Load the scene.
	const std::string scenePath = Platform::GetString("Engine.ini", "Scene", "Path", "../Assets/Meshes/Sponza/Sponza.gltf");
	const SceneLoadRequest sceneLoadReq(scenePath, false);

	HandleSceneLoadRequest(engine, sceneLoadReq);

	// Create the directional light.
	auto lightEntity = ecs.CreateEntity("DirectionalLight");
	auto& directionalLight = ecs.AddComponent(lightEntity, DirectionalLight());

	directionalLight.Intensity = Platform::GetFloat("Engine.ini", "DirectionalLight", "Intensity", 5.0f);
	directionalLight.ShadowType = EShadowType::Soft;
	directionalLight.DepthBiasConstantFactor = 1.75f;
	directionalLight.DepthBiasSlopeFactor = 1.75f;

	const float x = Platform::GetFloat("Engine.ini", "DirectionalLight", "X", 1.0f);
	const float y = Platform::GetFloat("Engine.ini", "DirectionalLight", "Y", 1.0f);
	const float z = Platform::GetFloat("Engine.ini", "DirectionalLight", "Z", 1.0f);

	auto& transform = ecs.AddComponent(lightEntity, Transform(ecs, lightEntity, glm::vec3(0), glm::radians(glm::vec3(x, y, z))));

	// Create the skybox.
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

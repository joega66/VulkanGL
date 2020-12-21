#include "SceneSystem.h"
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/SkyboxComponent.h>
#include <Components/RenderSettings.h>
#include <Engine/Engine.h>
#include <Engine/AssetManager.h>

void SceneSystem::Start(Engine& engine)
{
	auto& ecs = engine._ECS;

	ecs.AddSingletonComponent<RenderSettings>();

	// Create the camera.
	auto cameraEntity = ecs.CreateEntity("Camera");
	ecs.AddComponent(cameraEntity, Camera(
		glm::vec3(Platform::GetFloat("Engine.ini", "Camera", "LookFromX", 0.0f),
				  Platform::GetFloat("Engine.ini", "Camera", "LookFromY", 0.0f),
				  Platform::GetFloat("Engine.ini", "Camera", "LookFromZ", 0.0f)),
		glm::vec3(Platform::GetFloat("Engine.ini", "Camera", "LookAtX", 0.0f),
				  Platform::GetFloat("Engine.ini", "Camera", "LookAtY", 0.0f),
				  Platform::GetFloat("Engine.ini", "Camera", "LookAtZ", 0.0f))));

	_ScreenResizeEvent = engine._Screen.OnScreenResize([&] (uint32 width, uint32 height)
	{
		for (auto entity : ecs.GetEntities<Camera>())
		{
			auto& camera = ecs.GetComponent<Camera>(entity);
			camera.Resize(width, height);
		}
	});

	auto sceneLoadEntity = ecs.CreateEntity();
	ecs.AddComponent(sceneLoadEntity, SceneLoadRequest(Platform::GetString("Engine.ini", "Scene", "Path", "../Assets/Meshes/Sponza/Sponza.gltf"), false));

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

	ecs.AddComponent(lightEntity, Transform(ecs, lightEntity, glm::vec3(0), glm::radians(glm::vec3(x, y, z))));

	// Create the skybox.
	auto skyboxEntity = ecs.CreateEntity("Skybox");
	Skybox* skybox = engine.Assets.LoadSkybox("Default_Skybox", Platform::GetString("Engine.ini", "Scene", "Skybox", "../Assets/Cube_Maps/White_Cliff_Top/"));
	ecs.AddComponent(skyboxEntity, SkyboxComponent(skybox));
}

void SceneSystem::Update(Engine& engine)
{
	auto& ecs = engine._ECS;

	for (auto& entity : ecs.GetEntities<SceneLoadRequest>())
	{
		const auto& sceneLoadRequest = ecs.GetComponent<SceneLoadRequest>(entity);
		
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
			auto entity = ecs.CreateEntity(staticMesh->_Name.c_str());
			ecs.AddComponent(entity, StaticMeshComponent(staticMesh, staticMesh->_Materials.front()));

			auto& transform = ecs.GetComponent<Transform>(entity);
			transform.Scale(ecs, glm::vec3(scale));
		}

		ecs.Destroy(entity);
	}
}
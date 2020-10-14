#include "ShadowSystem.h"
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Renderer/ShadowProxy.h>
#include <Engine/Engine.h>

void ShadowSystem::Start(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine._Device;

	ecs.OnComponentCreated<DirectionalLight>([&] (Entity& entity, DirectionalLight& directionalLight)
	{
		ecs.AddComponent(entity, ShadowProxy(device, directionalLight));
	});
}

void ShadowSystem::Update(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine._Device;

	for (auto entity : ecs.GetEntities<ShadowProxy>())
	{
		const auto& directionalLight = ecs.GetComponent<DirectionalLight>(entity);
		const auto& transform = ecs.GetComponent<Transform>(entity);
		auto& shadowProxy = ecs.GetComponent<ShadowProxy>(entity);

		shadowProxy.Update(device, directionalLight, transform);
	}
}
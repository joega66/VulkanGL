#include "RenderSystem.h"
#include <Engine/Engine.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/RenderSettings.h>
#include <Components/Bounds.h>
#include <Renderer/MeshProxy.h>
#include <Renderer/ShadowProxy.h>

BEGIN_UNIFORM_BUFFER(LocalToWorldUniform)
	MEMBER(glm::mat4, transform)
	MEMBER(glm::mat4, inverse)
	MEMBER(glm::mat4, inverseTranspose)
END_UNIFORM_BUFFER(LocalToWorldUniform)

BEGIN_DESCRIPTOR_SET(StaticMeshDescriptors)
	DESCRIPTOR(gpu::UniformBuffer<LocalToWorldUniform>, _LocalToWorldUniform)
END_DESCRIPTOR_SET(StaticMeshDescriptors)

void RenderSystem::Start(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine.Device;

	ecs.AddSingletonComponent<RenderSettings>();

	ecs.OnComponentCreated<StaticMeshComponent>([&] (Entity& entity, StaticMeshComponent& staticMeshComponent)
	{
		const StaticMesh* staticMesh = staticMeshComponent.StaticMesh;

		gpu::Buffer localToWorldUniform = device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(LocalToWorldUniform));

		StaticMeshDescriptors surfaceDescriptors;
		surfaceDescriptors._LocalToWorldUniform = localToWorldUniform;

		gpu::DescriptorSet surfaceSet = device.CreateDescriptorSet(surfaceDescriptors);

		ecs.AddComponent(entity,
			MeshProxy(
				staticMeshComponent.Material,
				std::move(surfaceSet),
				staticMesh->Submeshes,
				std::move(localToWorldUniform))
		);

		auto& bounds = ecs.AddComponent(entity, Bounds());
		const auto& transform = ecs.GetComponent<Transform>(entity);
		bounds.Box = staticMesh->GetBounds().Transform(transform.GetLocalToWorld());
	});

	ecs.OnComponentCreated<DirectionalLight>([&] (Entity& entity, DirectionalLight& directionalLight)
	{
		ecs.AddComponent(entity, ShadowProxy(device, directionalLight));
	});
}

void RenderSystem::Update(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine.Device;

	for (auto entity : ecs.GetEntities<ShadowProxy>())
	{
		const auto& directionalLight = ecs.GetComponent<DirectionalLight>(entity);
		const auto& transform = ecs.GetComponent<Transform>(entity);
		auto& shadowProxy = ecs.GetComponent<ShadowProxy>(entity);
		
		shadowProxy.Update(device, directionalLight, transform);
	}

	for (auto& entity : ecs.GetEntities<MeshProxy>())
	{
		auto& meshProxy = ecs.GetComponent<MeshProxy>(entity);
		const auto& transform = ecs.GetComponent<Transform>(entity);
		auto& bounds = ecs.GetComponent<Bounds>(entity);
		const StaticMesh* staticMesh = ecs.GetComponent<StaticMeshComponent>(entity).StaticMesh;

		bounds.Box = staticMesh->GetBounds().Transform(transform.GetLocalToWorld());

		auto* localToWorldUniformBuffer = static_cast<LocalToWorldUniform*>(meshProxy._LocalToWorldUniform.GetData());
		localToWorldUniformBuffer->transform = transform.GetLocalToWorld();
		localToWorldUniformBuffer->inverse = glm::inverse(transform.GetLocalToWorld());
		localToWorldUniformBuffer->inverseTranspose = glm::transpose(localToWorldUniformBuffer->inverse);

		// Add to the light's shadow depth rendering.
		for (auto entity : ecs.GetEntities<ShadowProxy>())
		{
			auto& shadowProxy = ecs.GetComponent<ShadowProxy>(entity);
			shadowProxy.AddMesh(engine.Device, engine.ShaderLibrary, meshProxy);
		}
	}
}
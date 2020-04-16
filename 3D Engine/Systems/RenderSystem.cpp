#include "RenderSystem.h"
#include <Engine/Engine.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/RenderSettings.h>
#include <Components/Bounds.h>
#include <Renderer/MeshProxy.h>
#include <Renderer/ShadowProxy.h>

UNIFORM_STRUCT(LocalToWorldUniformBuffer,
	glm::mat4 Transform;
	glm::mat4 Inverse;
	glm::mat4 InverseTranspose;
);

RenderSystem::RenderSystem(Engine& Engine)
	: StaticMeshLayout(Engine.Device)
	, ShadowLayout(Engine.Device)
{
}

void RenderSystem::Start(Engine& Engine)
{
	auto& ECS = Engine.ECS;
	auto& Device = Engine.Device;

	ECS.AddSingletonComponent<RenderSettings>();

	ECS.NewComponentCallback<StaticMeshComponent>([&] (Entity& Entity, StaticMeshComponent& StaticMeshComponent)
	{
		const StaticMesh* StaticMesh = StaticMeshComponent.StaticMesh;
		drm::Buffer LocalToWorldUniform = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(LocalToWorldUniformBuffer));
		StaticMeshDescriptors SurfaceDescriptors = { LocalToWorldUniform };
		drm::DescriptorSet SurfaceSet = StaticMeshLayout.CreateDescriptorSet(Device);
		StaticMeshLayout.UpdateDescriptorSet(Device, SurfaceSet, SurfaceDescriptors);

		ECS.AddComponent(Entity,
			MeshProxy(
				StaticMeshComponent.Material,
				std::move(SurfaceSet),
				StaticMesh->Submeshes,
				std::move(LocalToWorldUniform))
		);

		auto& SurfaceBounds = ECS.AddComponent(Entity, Bounds());
		const auto& Transform = ECS.GetComponent<class Transform>(Entity);
		SurfaceBounds.Box = StaticMesh->GetBounds().Transform(Transform.GetLocalToWorld());
	});

	ECS.NewComponentCallback<DirectionalLight>([&] (Entity& Entity, DirectionalLight& DirectionalLight)
	{
		ECS.AddComponent(Entity, ShadowProxy(Device, ShadowLayout, DirectionalLight));
		auto& Box = ECS.AddComponent(Entity, Bounds()).Box;
		Box = BoundingBox(glm::vec3(0.0f), glm::vec3(0.0f));
	});
}

void RenderSystem::Update(Engine& Engine)
{
	auto& ECS = Engine.ECS;
	auto& Device = Engine.Device;

	for (auto Entity : ECS.GetEntities<ShadowProxy>())
	{
		auto& DirectionalLight = ECS.GetComponent<struct DirectionalLight>(Entity);
		auto& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
		ShadowProxy.Update(Device, DirectionalLight);
	}

	for (auto& Entity : ECS.GetEntities<MeshProxy>())
	{
		auto& MeshProxy = ECS.GetComponent<class MeshProxy>(Entity);
		const auto& Transform = ECS.GetComponent<class Transform>(Entity);
		auto& Bounds = ECS.GetComponent<class Bounds>(Entity);
		const auto* StaticMesh = ECS.GetComponent<class StaticMeshComponent>(Entity).StaticMesh;

		Bounds.Box = StaticMesh->GetBounds().Transform(Transform.GetLocalToWorld());

		LocalToWorldUniformBuffer* LocalToWorldUniformBuffer = static_cast<struct LocalToWorldUniformBuffer*>(MeshProxy.LocalToWorldUniform.GetData());
		LocalToWorldUniformBuffer->Transform = Transform.GetLocalToWorld();
		LocalToWorldUniformBuffer->Inverse = glm::inverse(Transform.GetLocalToWorld());
		LocalToWorldUniformBuffer->InverseTranspose = glm::transpose(LocalToWorldUniformBuffer->Inverse);

		// Add to the light's shadow depth rendering.
		for (auto Entity : ECS.GetEntities<ShadowProxy>())
		{
			auto& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
			ShadowProxy.AddMesh(Engine.Device, Engine.ShaderMap, MeshProxy);
		}
	}
}
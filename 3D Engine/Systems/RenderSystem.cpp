#include "RenderSystem.h"
#include <Engine/Engine.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/RenderSettings.h>
#include <Renderer/MeshProxy.h>
#include <Renderer/ShadowProxy.h>

UNIFORM_STRUCT(LocalToWorldUniformBuffer,
	glm::mat4 Transform;
	glm::mat4 Inverse;
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
		drm::DescriptorSet SurfaceSet = StaticMeshLayout.CreateDescriptorSet();
		StaticMeshLayout.UpdateDescriptorSet(SurfaceSet, SurfaceDescriptors);

		ECS.AddComponent(Entity, 
			MeshProxy(
				StaticMeshComponent.Material,
				std::move(SurfaceSet),
				StaticMesh->Submeshes,
				std::move(LocalToWorldUniform))
			);
	});

	ECS.NewComponentCallback<DirectionalLight>([&] (Entity& Entity, DirectionalLight& DirectionalLight)
	{
		ECS.AddComponent(Entity, ShadowProxy(Device, ShadowLayout, DirectionalLight));
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
		const auto* StaticMesh = ECS.GetComponent<class StaticMeshComponent>(Entity).StaticMesh;

		LocalToWorldUniformBuffer* LocalToWorldUniformBuffer = static_cast<struct LocalToWorldUniformBuffer*>(Device.LockBuffer(MeshProxy.LocalToWorldUniform));
		LocalToWorldUniformBuffer->Transform = Transform.GetLocalToWorld();
		LocalToWorldUniformBuffer->Inverse = glm::inverse(Transform.GetLocalToWorld());
		Device.UnlockBuffer(MeshProxy.LocalToWorldUniform);

		MeshProxy.WorldSpaceBB = StaticMesh->GetBounds().Transform(Transform.GetLocalToWorld());

		// Add to the light's shadow depth rendering.
		for (auto Entity : ECS.GetEntities<ShadowProxy>())
		{
			auto& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
			ShadowProxy.AddMesh(Engine.Device, Engine.ShaderMap, MeshProxy);
		}
	}
}
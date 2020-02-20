#include "RenderSystem.h"
#include <Engine/Engine.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Renderer/MeshProxy.h>
#include <Renderer/ShadowProxy.h>

UNIFORM_STRUCT(LocalToWorldUniformBuffer,
	glm::mat4 Transform;
	glm::mat4 Inverse;
);

RenderSystem::RenderSystem(Engine& Engine)
	: StaticMeshTemplate(Engine.Device)
	, ShadowTemplate(Engine.Device)
{
}

void RenderSystem::Start(Engine& Engine)
{
	auto& ECS = Engine.ECS;
	auto& Device = Engine.Device;

	ECS.NewComponentCallback<StaticMeshComponent>([&] (Entity& Entity, StaticMeshComponent& StaticMeshComponent)
	{
		const StaticMesh* StaticMesh = StaticMeshComponent.StaticMesh;
		drm::Buffer LocalToWorldUniform = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::KeepCPUAccessible, sizeof(LocalToWorldUniformBuffer));
		StaticMeshDescriptors SurfaceDescriptors = { &LocalToWorldUniform };
		drm::DescriptorSetRef SurfaceSet = StaticMeshTemplate.CreateDescriptorSet();
		StaticMeshTemplate.UpdateDescriptorSet(SurfaceSet, SurfaceDescriptors);

		ECS.AddComponent(Entity, 
			MeshProxy(
				StaticMeshComponent.Material,
				SurfaceSet,
				StaticMesh->Submeshes,
				std::move(LocalToWorldUniform))
			);
	});

	ECS.NewComponentCallback<DirectionalLight>([&] (Entity& Entity, DirectionalLight& DirectionalLight)
	{
		ECS.AddComponent(Entity, ShadowProxy(Device, ShadowTemplate, DirectionalLight));
	});
}

void RenderSystem::Update(Engine& Engine)
{
	auto& ECS = Engine.ECS;
	auto& Device = Engine.Device;

	for (auto& Entity : ECS.GetEntities<MeshProxy>())
	{
		MeshProxy& MeshProxy = ECS.GetComponent<class MeshProxy>(Entity);
		const Transform& Transform = ECS.GetComponent<class Transform>(Entity);
		const StaticMesh* StaticMesh = ECS.GetComponent<class StaticMeshComponent>(Entity).StaticMesh;

		LocalToWorldUniformBuffer* LocalToWorldUniformBuffer = static_cast<struct LocalToWorldUniformBuffer*>(Device.LockBuffer(MeshProxy.LocalToWorldUniform));
		LocalToWorldUniformBuffer->Transform = Transform.GetLocalToWorld();
		LocalToWorldUniformBuffer->Inverse = glm::inverse(Transform.GetLocalToWorld());
		Device.UnlockBuffer(MeshProxy.LocalToWorldUniform);

		MeshProxy.WorldSpaceBB = StaticMesh->GetBounds().Transform(Transform.GetLocalToWorld());
	}

	for (auto Entity : ECS.GetEntities<ShadowProxy>())
	{
		auto& DirectionalLight = ECS.GetComponent<struct DirectionalLight>(Entity);
		auto& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
		ShadowProxy.Update(Device, DirectionalLight);
	}
}
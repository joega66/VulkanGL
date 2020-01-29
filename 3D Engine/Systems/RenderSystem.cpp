#include "RenderSystem.h"
#include <Engine/Scene.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Renderer/MeshProxy.h>
#include <Renderer/ShadowProxy.h>

UNIFORM_STRUCT(LocalToWorldUniformBuffer,
	glm::mat4 Transform;
	glm::mat4 Inverse;
);

void RenderSystem::Start(EntityManager& ECS, DRMDevice& Device)
{
	ECS.NewComponentCallback<StaticMeshComponent>([&] (Entity& Entity, StaticMeshComponent& StaticMeshComponent)
	{
		const drm::BufferRef LocalToWorldUniform = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::KeepCPUAccessible, sizeof(LocalToWorldUniformBuffer));
		const StaticMesh* StaticMesh = ECS.GetComponent<class StaticMeshComponent>(Entity).StaticMesh;
		const Material& Material = ECS.GetComponent<class Material>(Entity);

		drm::DescriptorSetRef SurfaceSet = Device.CreateDescriptorSet();
		SurfaceSet->Write(LocalToWorldUniform, 0);
		SurfaceSet->Update();

		ECS.AddComponent<MeshProxy>(Entity, MeshProxy(Device, SurfaceSet, Material, StaticMesh->Submeshes, LocalToWorldUniform));
	});

	ECS.NewComponentCallback<DirectionalLight>([&] (Entity& Entity, DirectionalLight& DirectionalLight)
	{
		ECS.AddComponent<ShadowProxy>(Entity, ShadowProxy(Device, DirectionalLight));
	});
}

void RenderSystem::Update(EntityManager& ECS, DRMDevice& Device)
{
	for (auto& Entity : ECS.GetEntities<MeshProxy>())
	{
		MeshProxy& MeshProxy = ECS.GetComponent<class MeshProxy>(Entity);
		const Transform& Transform = ECS.GetComponent<class Transform>(Entity);
		const StaticMesh* StaticMesh = ECS.GetComponent<class StaticMeshComponent>(Entity).StaticMesh;

		LocalToWorldUniformBuffer* LocalToWorldUniformBuffer = static_cast<struct LocalToWorldUniformBuffer*>(Device.LockBuffer(MeshProxy.LocalToWorldUniform));
		LocalToWorldUniformBuffer->Transform = Transform.GetLocalToWorld();
		LocalToWorldUniformBuffer->Inverse = glm::inverse(Transform.GetLocalToWorld());
		Device.UnlockBuffer(MeshProxy.LocalToWorldUniform);

		MeshProxy.WorldSpaceBB = StaticMesh->Bounds.Transform(Transform.GetLocalToWorld());
	}

	for (auto Entity : ECS.GetEntities<ShadowProxy>())
	{
		auto& DirectionalLight = ECS.GetComponent<struct DirectionalLight>(Entity);
		auto& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
		ShadowProxy.Update(Device, DirectionalLight);
	}
}
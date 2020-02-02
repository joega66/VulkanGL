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

RenderSystem::RenderSystem(DRMDevice& Device)
	: MaterialTemplate(Device)
	, StaticMeshTemplate(Device)
	, ShadowTemplate(Device)
{
}

void RenderSystem::Start(EntityManager& ECS, DRMDevice& Device)
{
	ECS.NewComponentCallback<StaticMeshComponent>([&] (Entity& Entity, StaticMeshComponent& StaticMeshComponent)
	{
		const StaticMesh* StaticMesh = StaticMeshComponent.StaticMesh;

		StaticMeshDescriptors StaticMeshDescriptors = { Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::KeepCPUAccessible, sizeof(LocalToWorldUniformBuffer)) };

		drm::DescriptorSetRef SurfaceSet = StaticMeshTemplate.CreateDescriptorSet();

		StaticMeshTemplate.UpdateDescriptorSet(SurfaceSet, StaticMeshDescriptors);

		Material& Material = ECS.GetComponent<class Material>(Entity);

		drm::DescriptorSetRef MaterialSet = MaterialTemplate.CreateDescriptorSet();

		MaterialTemplate.UpdateDescriptorSet(MaterialSet, Material.Descriptors);

		ECS.AddComponent<MeshProxy>(Entity, MeshProxy(Device, SurfaceSet, Material, MaterialSet, StaticMesh->Submeshes, StaticMeshDescriptors.LocalToWorldUniform));
	});

	ECS.NewComponentCallback<DirectionalLight>([&] (Entity& Entity, DirectionalLight& DirectionalLight)
	{
		ECS.AddComponent<ShadowProxy>(Entity, ShadowProxy(Device, ShadowTemplate, DirectionalLight));
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
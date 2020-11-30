#include "SurfaceSystem.h"
#include <Engine/Engine.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Renderer/Surface.h>

DECLARE_UNIFORM_BUFFER(LocalToWorldUniform)

DECLARE_DESCRIPTOR_SET(StaticMeshDescriptors)

void SurfaceSystem::Start(Engine& engine)
{
	auto& device = engine._Device;
}

void SurfaceSystem::Update(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine._Device;

	for (auto entity : ecs.GetEntities<SurfaceGroup>())
	{
		ecs.Destroy(entity);
	}

	auto entities = ecs.GetEntities<StaticMeshComponent>();

	_SurfaceBuffer = device.CreateBuffer(EBufferUsage::Storage, EMemoryUsage::CPU_TO_GPU, entities.size() * sizeof(LocalToWorldUniform));
	
	StaticMeshDescriptors descriptors;
	descriptors._LocalToWorldBuffer = _SurfaceBuffer;

	descriptors.Update();

	auto surfaceGroupEntity = ecs.CreateEntity();
	auto& surfaceGroup = ecs.AddComponent(surfaceGroupEntity, SurfaceGroup(StaticMeshDescriptors::_DescriptorSet));

	uint32 surfaceId = 0;

	for (auto& entity : entities)
	{
		const auto& staticMesh = ecs.GetComponent<StaticMeshComponent>(entity);
		const auto& transform = ecs.GetComponent<Transform>(entity);
		const BoundingBox boundingBox = staticMesh.StaticMesh->GetBounds().Transform(transform.GetLocalToWorld());

		auto* localToWorldUniformBuffer = reinterpret_cast<LocalToWorldUniform*>(_SurfaceBuffer.GetData()) + surfaceId;
		localToWorldUniformBuffer->transform = transform.GetLocalToWorld();
		localToWorldUniformBuffer->inverse = glm::inverse(transform.GetLocalToWorld());
		localToWorldUniformBuffer->inverseTranspose = glm::transpose(localToWorldUniformBuffer->inverse);

		surfaceGroup.AddSurface(Surface(surfaceId++, staticMesh.Material, staticMesh.StaticMesh->Submeshes, boundingBox));
	}
}
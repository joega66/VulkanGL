#include "RenderSystem.h"
#include <Engine/Engine.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/RenderSettings.h>
#include <Components/Bounds.h>
#include <Renderer/MeshProxy.h>
#include <Renderer/ShadowProxy.h>
#include <Renderer/CameraProxy.h>

BEGIN_UNIFORM_BUFFER(LocalToWorldUniform)
	MEMBER(glm::mat4, transform)
	MEMBER(glm::mat4, inverse)
	MEMBER(glm::mat4, inverseTranspose)
END_UNIFORM_BUFFER(LocalToWorldUniform)

BEGIN_DESCRIPTOR_SET(StaticMeshDescriptors)
	DESCRIPTOR(gpu::StorageBuffer, _LocalToWorldBuffer)
END_DESCRIPTOR_SET(StaticMeshDescriptors)

void RenderSystem::Start(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine._Device;
	auto& surface = engine.Surface;
	auto& screen = engine._Screen;

	ecs.AddSingletonComponent<RenderSettings>();

	_SurfaceSet = device.CreateDescriptorSet<StaticMeshDescriptors>();

	ecs.OnComponentCreated<Camera>([&] (Entity& entity, Camera& camera)
	{
		ecs.AddComponent(entity, CameraProxy(screen, device, surface));
	});

	ecs.OnComponentCreated<StaticMeshComponent>([&] (Entity& entity, StaticMeshComponent& staticMeshComponent)
	{
		const StaticMesh* staticMesh = staticMeshComponent.StaticMesh;

		ecs.AddComponent(entity,
			MeshProxy(
				staticMeshComponent.Material,
				staticMesh->Submeshes
			)
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
	auto& device = engine._Device;

	for (auto entity : ecs.GetEntities<ShadowProxy>())
	{
		const auto& directionalLight = ecs.GetComponent<DirectionalLight>(entity);
		const auto& transform = ecs.GetComponent<Transform>(entity);
		auto& shadowProxy = ecs.GetComponent<ShadowProxy>(entity);
		
		shadowProxy.Update(device, directionalLight, transform);
	}

	auto entities = ecs.GetEntities<MeshProxy>();

	_SurfaceBuffer = device.CreateBuffer(EBufferUsage::Storage, EMemoryUsage::CPU_TO_GPU, entities.size() * sizeof(LocalToWorldUniform));
	
	StaticMeshDescriptors descriptors;
	descriptors._LocalToWorldBuffer = _SurfaceBuffer;

	device.UpdateDescriptorSet(_SurfaceSet, descriptors);

	uint32 surfaceId = 0;

	for (auto& entity : entities)
	{
		auto& meshProxy = ecs.GetComponent<MeshProxy>(entity);
		const auto& transform = ecs.GetComponent<Transform>(entity);
		auto& bounds = ecs.GetComponent<Bounds>(entity);
		const auto staticMesh = ecs.GetComponent<StaticMeshComponent>(entity).StaticMesh;

		bounds.Box = staticMesh->GetBounds().Transform(transform.GetLocalToWorld());

		auto* localToWorldUniformBuffer = reinterpret_cast<LocalToWorldUniform*>(_SurfaceBuffer.GetData()) + surfaceId;
		localToWorldUniformBuffer->transform = transform.GetLocalToWorld();
		localToWorldUniformBuffer->inverse = glm::inverse(transform.GetLocalToWorld());
		localToWorldUniformBuffer->inverseTranspose = glm::transpose(localToWorldUniformBuffer->inverse);

		meshProxy._SurfaceSet = &_SurfaceSet;
		meshProxy._SurfaceID = surfaceId++;

		// Add to the light's shadow depth rendering.
		for (auto entity : ecs.GetEntities<ShadowProxy>())
		{
			auto& shadowProxy = ecs.GetComponent<ShadowProxy>(entity);
			shadowProxy.AddMesh(engine._Device, engine.ShaderLibrary, meshProxy);
		}
	}

	for (auto entity : ecs.GetEntities<Camera>())
	{
		auto& camera = ecs.GetComponent<Camera>(entity);
		auto& cameraProxy = ecs.GetComponent<CameraProxy>(entity);

		cameraProxy.Update(camera, engine);
	}
}
#include "RenderSystem.h"
#include <Engine/Engine.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/RenderSettings.h>
#include <Renderer/Surface.h>
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
	auto& compositor = engine._Compositor;
	auto& screen = engine._Screen;

	ecs.AddSingletonComponent<RenderSettings>();

	_SurfaceSet = device.CreateDescriptorSet<StaticMeshDescriptors>();

	ecs.OnComponentCreated<Camera>([&] (Entity& entity, Camera& camera)
	{
		auto& renderCamera = ecs.AddComponent(entity, CameraProxy(device));
		renderCamera.Resize(device, screen.GetWidth(), screen.GetHeight());
	});

	ecs.OnComponentCreated<DirectionalLight>([&] (Entity& entity, DirectionalLight& directionalLight)
	{
		ecs.AddComponent(entity, ShadowProxy(device, directionalLight));
	});

	_ScreenResizeEvent = screen.OnScreenResize([&] (uint32 width, uint32 height)
	{
		for (auto entity : ecs.GetEntities<CameraProxy>())
		{
			auto& renderCamera = ecs.GetComponent<CameraProxy>(entity);
			renderCamera.Resize(device, width, height);
		}
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

	for (auto entity : ecs.GetEntities<SurfaceGroup>())
	{
		ecs.Destroy(entity);
	}

	auto entities = ecs.GetEntities<StaticMeshComponent>();

	_SurfaceBuffer = device.CreateBuffer(EBufferUsage::Storage, EMemoryUsage::CPU_TO_GPU, entities.size() * sizeof(LocalToWorldUniform));
	
	StaticMeshDescriptors descriptors;
	descriptors._LocalToWorldBuffer = _SurfaceBuffer;

	device.UpdateDescriptorSet(_SurfaceSet, descriptors);

	auto surfaceGroupEntity = ecs.CreateEntity();
	auto& surfaceGroup = ecs.AddComponent(surfaceGroupEntity, SurfaceGroup(_SurfaceSet));

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

	for (auto entity : ecs.GetEntities<Camera>())
	{
		auto& camera = ecs.GetComponent<Camera>(entity);
		auto& cameraProxy = ecs.GetComponent<CameraProxy>(entity);

		cameraProxy.Update(camera);
	}
}
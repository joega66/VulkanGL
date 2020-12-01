#include "CameraSystem.h"
#include <Engine/Engine.h>
#include <Renderer/CameraProxy.h>

DECLARE_UNIFORM_BUFFER(CameraUniform)

DECLARE_DESCRIPTOR_SET(CameraDescriptors);

void CameraSystem::Start(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine._Device;
	auto& screen = engine._Screen;

	ecs.OnComponentCreated<Camera>([&] (Entity& entity, Camera& camera)
	{
		auto& renderCamera = ecs.AddComponent(entity, CameraProxy(device));
		renderCamera.Resize(device, screen.GetWidth(), screen.GetHeight());
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

void CameraSystem::Update(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine._Device;

	auto entities = ecs.GetEntities<Camera>();

	_CameraUniform = device.CreateBuffer(EBufferUsage::Uniform, EMemoryUsage::CPU_TO_GPU, entities.size() * sizeof(CameraUniform));

	auto cameraUniformData = static_cast<CameraUniform*>(_CameraUniform.GetData());

	int i = 0;

	for (auto entity : ecs.GetEntities<Camera>())
	{
		auto& camera = ecs.GetComponent<Camera>(entity);
		auto& cameraProxy = ecs.GetComponent<CameraProxy>(entity);

		const glm::vec3 clipData(
			camera.GetFarPlane() * camera.GetNearPlane(),
			camera.GetNearPlane() - camera.GetFarPlane(),
			camera.GetFarPlane());

		const glm::mat4 clipToWorld = glm::inverse(camera.GetWorldToClip());

		const CameraUniform cameraUniform =
		{
			camera.GetWorldToView(),
			camera.GetViewToClip(),
			camera.GetWorldToClip(),
			clipToWorld,
			camera.GetPrevWorldToClip(),
			camera.GetPosition(),
			0.0f,
			camera.GetAspectRatio(),
			camera.GetFieldOfView(),
			glm::vec2(camera.GetWidth(), camera.GetHeight()),
			clipData,
			0.0f,
		};

		cameraUniformData[i] = cameraUniform;

		cameraProxy.SetDynamicOffset(i * sizeof(CameraUniform));

		const gpu::Sampler sampler = device.CreateSampler({ EFilter::Nearest });

		CameraDescriptors descriptors;
		descriptors._CameraUniform = { _CameraUniform };
		descriptors._SceneDepth = { cameraProxy._SceneDepth, sampler };
		descriptors._GBuffer0 = { cameraProxy._GBuffer0, sampler };
		descriptors._GBuffer1 = { cameraProxy._GBuffer1, sampler };
		descriptors._SceneColor = cameraProxy._SceneColor;
		descriptors._SSRHistory = cameraProxy._SSRHistory;
		descriptors._SSGIHistory = cameraProxy._SSGIHistory;
		descriptors._DirectLighting = cameraProxy._DirectLighting;

		device.UpdateDescriptorSet(descriptors);

		i++;
	}
}
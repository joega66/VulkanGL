#include "CameraSystem.h"
#include <Engine/Engine.h>
#include <Renderer/CameraRender.h>

DECLARE_UNIFORM_BUFFER(CameraUniform)

DECLARE_DESCRIPTOR_SET(CameraDescriptors);

void CameraSystem::Start(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine._Device;
	auto& screen = engine._Screen;

	ecs.OnComponentCreated<Camera>([&] (Entity& entity, Camera& camera)
	{
		auto& cameraRender = ecs.AddComponent(entity, CameraRender());
		cameraRender.Resize(device, screen.GetWidth(), screen.GetHeight());
	});

	_ScreenResizeEvent = screen.OnScreenResize([&] (uint32 width, uint32 height)
	{
		for (auto entity : ecs.GetEntities<CameraRender>())
		{
			auto& cameraRender = ecs.GetComponent<CameraRender>(entity);
			cameraRender.Resize(device, width, height);
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
		auto& cameraRender = ecs.GetComponent<CameraRender>(entity);

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

		cameraRender.SetDynamicOffset(i * sizeof(CameraUniform));

		const gpu::Sampler sampler = device.CreateSampler({ EFilter::Nearest });

		CameraDescriptors descriptors;
		descriptors._CameraUniform = { _CameraUniform };
		descriptors._SceneDepth = { cameraRender._SceneDepth, sampler };
		descriptors._GBuffer0 = { cameraRender._GBuffer0, sampler };
		descriptors._GBuffer1 = { cameraRender._GBuffer1, sampler };
		descriptors._SceneColor = cameraRender._SceneColor;
		descriptors._SSRHistory = cameraRender._SSRHistory;
		descriptors._SSGIHistory = cameraRender._SSGIHistory;
		descriptors._DirectLighting = cameraRender._DirectLighting;

		device.UpdateDescriptorSet(descriptors);

		i++;
	}
}
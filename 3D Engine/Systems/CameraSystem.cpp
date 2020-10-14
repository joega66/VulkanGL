#include "CameraSystem.h"
#include <Engine/Engine.h>
#include <Renderer/CameraProxy.h>

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

	for (auto entity : ecs.GetEntities<Camera>())
	{
		auto& camera = ecs.GetComponent<Camera>(entity);
		auto& cameraProxy = ecs.GetComponent<CameraProxy>(entity);

		cameraProxy.Update(camera);
	}
}
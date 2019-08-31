#include "LightSystem.h"
#include <Renderer/Scene.h>
#include <Renderer/Light.h>
#include <Components/CLight.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>

void LightSystem::Update()
{
	auto& Scene = Scene::Get();

	std::vector<PointLightProxy> PointLightProxies;

	for (auto Entity : GEntityManager.GetEntities<CLight>())
	{
		auto& Light = Entity.GetComponent<CLight>();
		auto& Transform = Entity.GetComponent<CTransform>();
		auto& Renderer = Entity.GetComponent<CRenderer>();

		if (Renderer.bVisible)
		{
			PointLightProxies.emplace_back(PointLightProxy{ Transform.GetPosition(), Light.Intensity, Light.Color, Light.Range });
		}
	}

	glm::uvec4 NumPointLights;
	NumPointLights.x = PointLightProxies.size();

	Scene.PointLightBuffer = drm::CreateStorageBuffer(sizeof(NumPointLights) + sizeof(PointLightProxy) * PointLightProxies.size(), nullptr);
	
	void* Data = drm::LockBuffer(Scene.PointLightBuffer);
	Platform.Memcpy(Data, &NumPointLights.x, sizeof(NumPointLights.x));
	Platform.Memcpy((uint8*)Data + sizeof(NumPointLights), PointLightProxies.data(), sizeof(PointLightProxy) * PointLightProxies.size());
	drm::UnlockBuffer(Scene.PointLightBuffer);
}
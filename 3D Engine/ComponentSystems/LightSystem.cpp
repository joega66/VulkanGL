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
	
	Scene.PointLightBuffer = drm::CreateStorageBuffer(sizeof(PointLightProxy) * PointLightProxies.size(), PointLightProxies.data());
}
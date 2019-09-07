#include "LightSystem.h"
#include <Renderer/Scene.h>
#include <Renderer/Light.h>
#include <Components/CLight.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include <ECS/EntityManager.h>

void LightSystem::Update(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	Scene.PointLightProxies.clear();

	for (auto Entity : ECS.GetEntities<CLight>())
	{
		auto& Light = ECS.GetComponent<CLight>(Entity);
		auto& Transform = ECS.GetComponent<CTransform>(Entity);
		auto& Renderer = ECS.GetComponent<CRenderer>(Entity);

		if (Renderer.bVisible)
		{
			Scene.PointLightProxies.emplace_back(PointLightProxy{ Transform.GetPosition(), Light.Intensity, Light.Color, Light.Range });
		}
	}
}
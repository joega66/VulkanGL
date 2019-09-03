#include "LightSystem.h"
#include <Renderer/Scene.h>
#include <Renderer/Light.h>
#include <Components/CLight.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include <ECS/EntityManager.h>

void LightSystem::Update(Scene& Scene)
{
	Scene.PointLightProxies.clear();

	for (auto Entity : GEntityManager.GetEntities<CLight>())
	{
		auto& Light = GEntityManager.GetComponent<CLight>(Entity);
		auto& Transform = GEntityManager.GetComponent<CTransform>(Entity);
		auto& Renderer = GEntityManager.GetComponent<CRenderer>(Entity);

		if (Renderer.bVisible)
		{
			Scene.PointLightProxies.emplace_back(PointLightProxy{ Transform.GetPosition(), Light.Intensity, Light.Color, Light.Range });
		}
	}
}
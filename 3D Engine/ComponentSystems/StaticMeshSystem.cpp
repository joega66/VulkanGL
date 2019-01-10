#include "StaticMeshSystem.h"
#include <Components/Entity.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include <Renderer/Scene.h>

StaticMeshSystem::StaticMeshSystem()
{
}

void StaticMeshSystem::Update()
{
	auto& Scene = Scene::Get();
	
	for (auto Entity : GEntityManager.GetEntities<CStaticMesh>())
	{
		// @todo Move this to DrawingPlanList...?
		if (!Entity.GetComponent<CRenderer>().bVisible)
			continue;

		auto& StaticMesh = Entity.GetComponent<CStaticMesh>();
		auto& Transform = Entity.GetComponent<CTransform>();

		if (Entity.HasComponent<CMaterial>())
		{
			auto& Material = Entity.GetComponent<CMaterial>();
			StaticMesh.ForEach([&](auto& Resource)
			{
				Scene.LightingPassDrawingPlans.Add(Entity, LightingPassDrawingPlan(Resource, Material, Transform.LocalToWorldUniform));
			});
		}
		else
		{
			StaticMesh.ForEach([&](auto& Resource)
			{
				Scene.LightingPassDrawingPlans.Add(Entity, LightingPassDrawingPlan(Resource, Resource.Material, Transform.LocalToWorldUniform));
			});
		}
	}
}
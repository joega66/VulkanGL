#include "StaticMeshSystem.h"
#include <Components/Entity.h>
#include <Renderer/Scene.h>

StaticMeshSystem::StaticMeshSystem()
{
	Listen<CStaticMesh>();
}

void StaticMeshSystem::RenderUpdate()
{
	auto& Scene = Scene::Get();

	for (auto Entity : GEntityManager.GetEntitiesForRenderUpdate<CStaticMesh>())
	{
		auto& Component = Entity.GetComponent<CStaticMesh>();
		auto& StaticMesh = Component.StaticMesh;
		auto& Materials = Component.Materials;
		auto& Transform = Entity.GetComponent<CTransform>();

		for (auto& Resource : StaticMesh->Resources)
		{
			MaterialProxy MaterialBatch;
			MaterialBatch.Merge(Resource.Materials);
			MaterialBatch.Merge(Materials);

			Scene.LightingPassDrawingPlans.Add(Entity, LightingPassDrawingPlan(Resource, MaterialBatch, Transform.LocalToWorldUniform));
		}
	}
}

void StaticMeshSystem::OnRemove(std::type_index Type, const Entity& Entity)
{
	auto& Scene = Scene::Get();
	Scene.LightingPassDrawingPlans.Remove(Entity);
}
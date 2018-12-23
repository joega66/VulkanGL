#include "StaticMeshSystem.h"
#include "Components/Entity.h"
#include "Renderer/Scene.h"

void StaticMeshSystem::RenderUpdate(Scene& Scene)
{
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
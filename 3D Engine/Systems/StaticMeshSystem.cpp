#include "StaticMeshSystem.h"
#include <ECS/EntityManager.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include <Renderer/Scene.h>

void StaticMeshSystem::Update(Scene& Scene)
{
	for (auto Entity : GEntityManager.GetEntities<CStaticMesh>())
	{
		if (!Entity.GetComponent<CRenderer>().bVisible)
			continue;

		auto& StaticMesh = Entity.GetComponent<CStaticMesh>();
		auto& Transform = Entity.GetComponent<CTransform>();

		if (Entity.HasComponent<CMaterial>())
		{
			// The entity has a Material -- render using it.
			auto& Material = Entity.GetComponent<CMaterial>();
			auto& Batch = StaticMesh.StaticMesh->Batch;
			auto& Elements = Batch.Elements;
			for (auto& Element : Elements)
			{
				Scene.LightingPass.Add(Entity, LightingPassDrawPlan(Element, Material, Transform.LocalToWorldUniform));
			}
		}
		else
		{
			// Use the materials that came with the static mesh.
			auto& Batch = StaticMesh.StaticMesh->Batch;
			auto& Elements = Batch.Elements;
			for (auto& Element : Elements)
			{
				Scene.LightingPass.Add(Entity, LightingPassDrawPlan(Element, Element.Material, Transform.LocalToWorldUniform));
			}
		}
	}
}
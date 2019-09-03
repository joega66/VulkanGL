#include "StaticMeshSystem.h"
#include <ECS/EntityManager.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include <Renderer/Scene.h>

void StaticMeshSystem::Update(Scene& Scene)
{
	for (auto Entity : GEntityManager.GetEntities<CStaticMesh>())
	{
		if (!GEntityManager.GetComponent<CRenderer>(Entity).bVisible)
			continue;

		auto& StaticMesh = GEntityManager.GetComponent<CStaticMesh>(Entity);
		auto& Transform = GEntityManager.GetComponent<CTransform>(Entity);

		if (GEntityManager.HasComponent<CMaterial>(Entity))
		{
			// The entity has a Material -- render using it.
			auto& Material = GEntityManager.GetComponent<CMaterial>(Entity);
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
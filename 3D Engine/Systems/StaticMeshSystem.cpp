#include "StaticMeshSystem.h"
#include <ECS/EntityManager.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include <Renderer/Scene.h>

void StaticMeshSystem::Update(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	for (auto Entity : ECS.GetEntities<CStaticMesh>())
	{
		if (!ECS.GetComponent<CRenderer>(Entity).bVisible)
			continue;

		auto& StaticMesh = ECS.GetComponent<CStaticMesh>(Entity);
		auto& Transform = ECS.GetComponent<CTransform>(Entity);

		if (ECS.HasComponent<CMaterial>(Entity))
		{
			// The entity has a Material -- render using it.
			auto& Material = ECS.GetComponent<CMaterial>(Entity);
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
#include "CStaticMesh.h"
#include "../Renderer/Scene.h"

CStaticMesh::CStaticMesh(StaticMeshRef StaticMesh, CTransformRef Transform)
	: StaticMesh(StaticMesh), Transform(Transform), Materials(MakeRef<MaterialProxy>())
{
}

void CStaticMesh::RenderUpdate(Scene* Scene)
{
	for (auto& Resource : StaticMesh->Resources)
	{
		MaterialProxyRef MaterialBatch = MakeRef<MaterialProxy>();
		
		MaterialBatch->Merge(Resource.Materials);
		MaterialBatch->Merge(Materials);

		Scene->LightingPassDrawingPlans.Add(LightingPassDrawingPlan(Resource, MaterialBatch, Transform->LocalToWorldUniform));
	}
}
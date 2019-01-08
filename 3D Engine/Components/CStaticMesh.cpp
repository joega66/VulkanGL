#include "CStaticMesh.h"

CStaticMesh::CStaticMesh(StaticMeshRef StaticMesh)
	: StaticMesh(StaticMesh)
{
}

void CStaticMesh::ForEach(const std::function<void(StaticMeshResources&)> Func)
{
	std::for_each(StaticMesh->Resources.begin(), StaticMesh->Resources.end(), Func);
}
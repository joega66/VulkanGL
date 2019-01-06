#include "CStaticMesh.h"
#include <Renderer/Scene.h>

CStaticMesh::CStaticMesh(StaticMeshRef StaticMesh)
	: StaticMesh(StaticMesh), Materials(MakeRef<MaterialProxy>())
{
}
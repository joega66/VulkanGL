#pragma once
#include <Engine/StaticMesh.h>

struct CStaticMesh
{
	CStaticMesh() = default;
	CStaticMesh(StaticMeshRef StaticMesh);
	StaticMeshRef StaticMesh;
};
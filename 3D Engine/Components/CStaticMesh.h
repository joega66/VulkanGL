#pragma once
#include <Engine/StaticMesh.h>

struct CStaticMesh : public Component<CStaticMesh>
{
	CStaticMesh() = default;
	CStaticMesh(StaticMeshRef StaticMesh);
	StaticMeshRef StaticMesh;
};

CLASS(CStaticMesh);
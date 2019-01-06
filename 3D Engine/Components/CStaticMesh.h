#pragma once
#include <Engine/StaticMesh.h>
#include "CTransform.h"

struct CStaticMesh : public Component<CStaticMesh, true>
{
	CStaticMesh() = default;
	CStaticMesh(StaticMeshRef StaticMesh);

	StaticMeshRef StaticMesh;
	MaterialProxyRef Materials;
};

CLASS(CStaticMesh);
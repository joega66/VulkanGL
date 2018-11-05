#pragma once
#include "../Engine/StaticMesh.h"
#include "CTransform.h"

class CStaticMesh : public Component
{
public:
	CStaticMesh(StaticMeshRef StaticMesh, CTransformRef Transform);

	virtual void RenderUpdate(class Scene* Scene) override;

private:
	StaticMeshRef StaticMesh;
	MaterialProxyRef Materials;
	CTransformRef Transform;
};

CLASS(CStaticMesh);
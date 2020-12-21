#pragma once
#include <ECS/Component.h>

class StaticMesh;
class Material;

class StaticMeshComponent : public Component
{
public:
	StaticMeshComponent(const StaticMesh* staticMesh, const Material* material);

	const StaticMesh* _StaticMesh = nullptr;
	const Material* _Material = nullptr;
};
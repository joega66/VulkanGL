#pragma once
#include <ECS/Component.h>

class StaticMesh;
class Material;

class StaticMeshComponent : public Component
{
public:
	StaticMeshComponent(const StaticMesh* StaticMesh, const Material* Material);

	const StaticMesh* StaticMesh = nullptr;
	const Material* Material = nullptr;
};
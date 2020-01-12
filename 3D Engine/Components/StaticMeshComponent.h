#pragma once
#include <Engine/StaticMesh.h>

class StaticMeshComponent
{
public:
	StaticMeshComponent() = default;

	StaticMeshComponent(const StaticMesh* StaticMesh);

	const StaticMesh* StaticMesh = nullptr;
};
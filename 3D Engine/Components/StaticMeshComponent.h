#pragma once

class StaticMesh;
class Material;

class StaticMeshComponent
{
public:
	StaticMeshComponent(const StaticMesh* StaticMesh, const Material* Material);

	const StaticMesh* StaticMesh = nullptr;
	const Material* Material = nullptr;
};
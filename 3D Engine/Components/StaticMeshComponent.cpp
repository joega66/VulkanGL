#include "StaticMeshComponent.h"

StaticMeshComponent::StaticMeshComponent(const class StaticMesh* StaticMesh, const class Material* Material)
	: StaticMesh(StaticMesh)
	, Material(Material)
{
}
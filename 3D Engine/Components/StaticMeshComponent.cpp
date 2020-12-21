#include "StaticMeshComponent.h"

StaticMeshComponent::StaticMeshComponent(const StaticMesh* staticMesh, const Material* material)
	: _StaticMesh(staticMesh)
	, _Material(material)
{
}
#pragma once
#include <DRM.h>
#include <Engine/StaticMesh.h>

class MeshProxy
{
public:
	const CMaterial& Material;

	drm::DescriptorSetRef MeshSet;

	drm::DescriptorSetRef MaterialSet;

	SpecializationInfo SpecializationInfo;

	drm::BufferRef LocalToWorldUniform;

	std::vector<MeshElement> Elements;

	MeshProxy(
		const CMaterial& Material, 
		const std::vector<MeshElement>& Elements,
		const drm::BufferRef& LocalToWorldUniform
	);
};
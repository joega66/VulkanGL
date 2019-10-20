#pragma once
#include <DRM.h>
#include <Engine/StaticMesh.h>

class MeshProxy
{
public:
	const CMaterial& Material;

	drm::DescriptorSetRef MaterialSet;

	SpecializationInfo SpecInfo;

	drm::UniformBufferRef LocalToWorldUniform;

	std::vector<MeshElement> Elements;

	MeshProxy(
		const CMaterial& Material, 
		const std::vector<MeshElement>& Elements,
		const drm::UniformBufferRef& LocalToWorldUniform
	);
};

CLASS(MeshProxy);
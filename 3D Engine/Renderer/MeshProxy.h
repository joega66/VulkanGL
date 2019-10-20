#pragma once
#include <DRM.h>
#include <Engine/StaticMesh.h>

class MeshProxy
{
public:
	const CMaterial& Material;

	std::vector<MeshElement> Elements;

	drm::DescriptorSetRef MaterialSet;

	SpecializationInfo SpecInfo;

	MeshProxy(
		const CMaterial& Material, 
		const std::vector<MeshElement>& Elements,
		const drm::UniformBufferRef& LocalToWorldUniform
	);
};

CLASS(MeshProxy);
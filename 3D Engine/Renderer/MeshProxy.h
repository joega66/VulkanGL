#pragma once
#include <DRM.h>
#include <Engine/StaticMesh.h>

class MeshProxy
{
public:
	const Material& Material;

	drm::DescriptorSetRef MeshSet;

	drm::DescriptorSetRef MaterialSet;

	SpecializationInfo SpecializationInfo;

	MeshProxy(
		const class Material& Material, 
		const std::vector<Submesh>& Submeshes,
		const drm::BufferRef& LocalToWorldUniform
	);

	void DrawElements(drm::CommandList& CmdList) const;

	inline const class SpecializationInfo& GetSpecializationInfo() const { return SpecializationInfo; }

	std::vector<Submesh> Submeshes;

private:
	drm::BufferRef LocalToWorldUniform;
};
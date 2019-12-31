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

	MeshProxy(
		const CMaterial& Material, 
		const std::vector<MeshElement>& Elements,
		const drm::BufferRef& LocalToWorldUniform
	);

	void DrawElements(drm::CommandList& CmdList) const;

	inline const class SpecializationInfo& GetSpecializationInfo() const { return SpecializationInfo; }

	std::vector<MeshElement> Elements;

private:
	drm::BufferRef LocalToWorldUniform;

};
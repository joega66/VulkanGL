#pragma once
#include <DRM.h>
#include <Engine/StaticMesh.h>

class MeshProxy
{
public:
	MeshProxy(
		DRMDevice& Device,
		drm::DescriptorSetRef SurfaceSet,
		const Material* Material,
		const std::vector<Submesh>& Submeshes,
		const drm::BufferRef& LocalToWorldUniform
	);

	void DrawElements(drm::CommandList& CmdList) const;

	inline const Material* GetMaterial() const { return Material; }
	inline const drm::DescriptorSetRef& GetSurfaceSet() const { return SurfaceSet; }
	inline const drm::DescriptorSetRef& GetMaterialSet() const { return Material->DescriptorSet; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return Material->GetSpecializationInfo(); }
	inline const std::vector<Submesh>& GetSubmeshes() const { return Submeshes; }

	/** World-space bounding box. */
	BoundingBox WorldSpaceBB;

	/** Local to world uniform. */
	drm::BufferRef LocalToWorldUniform;

private:
	const Material* Material;

	drm::DescriptorSetRef SurfaceSet;

	std::vector<Submesh> Submeshes;
};
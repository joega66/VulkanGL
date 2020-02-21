#pragma once
#include <DRM.h>
#include <Engine/StaticMesh.h>

class MeshProxy
{
public:
	MeshProxy(
		const Material* Material,
		drm::DescriptorSet&& SurfaceSet,
		const std::vector<Submesh>& Submeshes,
		drm::Buffer&& LocalToWorldUniform
	);

	void DrawElements(drm::CommandList& CmdList) const;

	inline const Material* GetMaterial() const { return Material; }
	inline const drm::DescriptorSet& GetSurfaceSet() const { return SurfaceSet; }
	inline const drm::DescriptorSet& GetMaterialSet() const { return Material->DescriptorSet; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return Material->GetSpecializationInfo(); }
	inline const std::vector<Submesh>& GetSubmeshes() const { return *Submeshes; }

	/** World-space bounding box. */
	BoundingBox WorldSpaceBB;

	/** Local to world uniform. */
	drm::Buffer LocalToWorldUniform;

private:
	const Material* Material;

	drm::DescriptorSet SurfaceSet;

	const std::vector<Submesh>* Submeshes;
};
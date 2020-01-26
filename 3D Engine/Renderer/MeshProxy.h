#pragma once
#include <DRM.h>
#include <Engine/StaticMesh.h>

class MeshProxy
{
public:
	MeshProxy() = default;

	MeshProxy(
		DRM& Device,
		drm::DescriptorSetRef SurfaceSet,
		const class Material& Material,
		const std::vector<Submesh>& Submeshes,
		const drm::BufferRef& LocalToWorldUniform
	);

	void DrawElements(drm::CommandList& CmdList) const;

	inline const Material* GetMaterial() const { return Material; }
	inline const drm::DescriptorSetRef& GetSurfaceSet() const { return SurfaceSet; }
	inline const drm::DescriptorSetRef& GetMaterialSet() const { return MaterialSet; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return SpecializationInfo; }
	inline const std::vector<Submesh>& GetSubmeshes() const { return Submeshes; }

	/** World-space bounding box. */
	BoundingBox WorldSpaceBB;

	/** Local to world uniform. */
	drm::BufferRef LocalToWorldUniform;

private:
	const Material* Material;

	drm::DescriptorSetRef SurfaceSet;

	drm::DescriptorSetRef MaterialSet;

	SpecializationInfo SpecializationInfo;

	std::vector<Submesh> Submeshes;

};
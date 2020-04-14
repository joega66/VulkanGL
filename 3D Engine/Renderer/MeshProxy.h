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

	inline const std::vector<Submesh>& GetSubmeshes() const { return *Submeshes; }
	inline const Material* GetMaterial() const { return Material; }
	inline const drm::DescriptorSet& GetSurfaceSet() const { return SurfaceSet; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return Material->GetSpecializationInfo(); }
	
	/** Local to world uniform. */
	drm::Buffer LocalToWorldUniform;

private:
	const std::vector<Submesh>* Submeshes;
	const Material* Material;
	drm::DescriptorSet SurfaceSet;
};
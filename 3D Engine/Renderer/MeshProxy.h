#pragma once
#include <GPU/GPU.h>
#include <ECS/Component.h>
#include <Engine/StaticMesh.h>

class MeshProxy : public Component
{
public:
	MeshProxy(
		const Material* Material,
		gpu::DescriptorSet&& SurfaceSet,
		const std::vector<Submesh>& Submeshes,
		gpu::Buffer&& LocalToWorldUniform
	);

	inline const std::vector<Submesh>& GetSubmeshes() const { return *Submeshes; }
	inline const Material* GetMaterial() const { return Material; }
	inline const gpu::DescriptorSet& GetSurfaceSet() const { return SurfaceSet; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return Material->GetSpecializationInfo(); }
	
	/** Local to world uniform. */
	gpu::Buffer LocalToWorldUniform;

private:
	const std::vector<Submesh>* Submeshes;
	const Material* Material;
	gpu::DescriptorSet SurfaceSet;
};
#pragma once
#include <GPU/GPU.h>
#include <ECS/Component.h>
#include <Engine/StaticMesh.h>

class MeshProxy : public Component
{
public:
	MeshProxy(
		const Material* material,
		const std::vector<Submesh>& submeshes
	);

	inline const std::vector<Submesh>& GetSubmeshes() const { return *_Submeshes; }
	inline const Material* GetMaterial() const { return _Material; }
	inline const gpu::DescriptorSet& GetSurfaceSet() const { return *_SurfaceSet; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return _Material->GetSpecializationInfo(); }
	
	const gpu::DescriptorSet* _SurfaceSet;
	uint32 _SurfaceID;

private:
	const std::vector<Submesh>* _Submeshes;
	const Material* _Material;
};
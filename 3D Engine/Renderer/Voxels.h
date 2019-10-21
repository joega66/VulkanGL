#pragma once
#include <DRM.h>

class VoxelizationPass
{
public:
	struct PassDescriptors
	{
		drm::DescriptorSetRef Scene;
		drm::DescriptorSetRef Voxels;
	};

	VoxelizationPass(const class MeshProxy& MeshProxy);

	void BindDescriptorSets(RenderCommandList& CmdList, const class MeshProxy& MeshProxy, const PassDescriptors& Pass) const;
	void SetPipelineState(PipelineStateInitializer& PSOInit, const class MeshProxy& MeshProxy) const;
	void Draw(RenderCommandList& CmdList, const struct MeshElement& MeshElement) const;

private:
	drm::ShaderRef VertShader;
	drm::ShaderRef GeomShader;
	drm::ShaderRef FragShader;
};

extern uint32 gVoxelGridSize;
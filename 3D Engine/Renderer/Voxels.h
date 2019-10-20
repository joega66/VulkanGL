#pragma once
#include <DRM.h>

class VoxelizationPass
{
public:
	VoxelizationPass(const struct MeshElement& Element, struct CMaterial& Material, drm::UniformBufferRef LocalToWorldUniform);

	void BindDescriptorSets(RenderCommandList& CmdList, const class SceneProxy& Scene) const;
	void SetPipelineState(PipelineStateInitializer& PSOInit) const;
	void Draw(RenderCommandList& CmdList) const;

private:
	drm::ShaderRef VertShader;
	drm::ShaderRef GeomShader;
	drm::ShaderRef FragShader;
	drm::DescriptorSetRef DescriptorSet;
	SpecializationInfo SpecInfo;
	const struct MeshElement& Element;
};

extern const glm::uvec2 gVoxelGridSize;
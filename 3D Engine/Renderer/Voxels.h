#pragma once
#include <DRM.h>

/** Voxel cone tracing lighting cache. */
class VCTLightingCache
{
public:
	VCTLightingCache(class Engine& Engine);

	/** Voxelize the scene. */
	void Render(SceneProxy& Scene, drm::CommandList& CmdList);

	/** Visualize the voxels. */
	void RenderVisualization(SceneRenderer& SceneRenderer, drm::CommandList& CmdList);

	inline uint32 GetVoxelGridSize() const { return VoxelGridSize; }
	inline const drm::RenderPass& GetRenderPass() const { return VoxelRP; }
	inline const drm::RenderPass& GetDebugRenderPass() const { return DebugRP; }
	inline const drm::DescriptorSet& GetDescriptorSet() const { return DescriptorSet; }
	inline bool IsDebuggingEnabled() const { return DebugVoxels; }

	/** Create the render pass used for debugging. */
	void CreateDebugRenderPass(const drm::Image& SceneColor, const drm::Image& SceneDepth);

private:
	const uint32 VoxelGridSize;
	const bool DebugVoxels;

	DRMDevice& Device;
	DRMShaderMap& ShaderMap;

	drm::RenderPass VoxelRP;
	drm::RenderPass DebugRP;

	drm::DescriptorSet DescriptorSet;
	drm::DescriptorSetLayout DescriptorSetLayout;

	drm::Buffer WorldToVoxelBuffer;
	drm::Image VoxelBaseColor;

	drm::Buffer VoxelPositions;
	drm::Buffer VoxelIndirectBuffer;

	void RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList);

	void CreateVoxelRP();
};
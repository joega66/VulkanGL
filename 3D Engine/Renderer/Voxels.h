#pragma once
#include <DRM.h>

struct VoxelDescriptors
{
	drm::BufferView WorldToVoxelBuffer;
	drm::ImageView VoxelColors;
	drm::BufferView VoxelPositions;
	drm::BufferView VoxelIndirectBuffer;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static const std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, UniformBuffer },
			{ 1, 1, StorageImage },
			{ 2, 1, StorageBuffer },
			{ 3, 1, StorageBuffer },
		};
		return Bindings;
	}
};

/** Voxel cone tracing lighting cache. */
class VCTLightingCache
{
public:
	VCTLightingCache(class Engine& Engine);

	/** Voxelize the scene. */
	void Render(SceneProxy& Scene, drm::CommandList& CmdList);

	/** Visualize the voxels. */
	void RenderVisualization(SceneRenderer& SceneRenderer, drm::CommandList& CmdList);

	/** Resize callback for voxel visualization. */
	void Resize(const drm::Image& SceneColor, const drm::Image& SceneDepth, const drm::DescriptorSet* CameraDescriptorSet);

	inline uint32 GetVoxelGridSize() const { return VoxelGridSize; }
	inline const drm::RenderPass& GetRenderPass() const { return VoxelRP; }
	inline const DescriptorSet<VoxelDescriptors>& GetDescriptorSet() const { return VoxelDescriptorSet; }

private:
	const uint32 VoxelGridSize;

	DRMDevice& Device;
	DRMShaderMap& ShaderMap;

	drm::RenderPass VoxelRP;
	drm::RenderPass VoxelVisualizationRP;

	DescriptorSet<VoxelDescriptors> VoxelDescriptorSet;

	drm::Buffer WorldToVoxelBuffer;
	drm::Image VoxelColors;
	drm::Buffer VoxelPositions;
	drm::Buffer VoxelIndirectBuffer;

	drm::Pipeline VoxelVisualizationPipeline;

	void RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList);

	void CreateVoxelRP();
	void CreateVoxelVisualizationRP(const drm::Image& SceneColor, const drm::Image& SceneDepth, const drm::DescriptorSet* CameraDescriptorSet);
};
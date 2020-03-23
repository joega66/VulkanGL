#pragma once
#include <DRM.h>

class SceneProxy;

class VoxelShader
{
public:
	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Worker.SetDefine("VOXEL_GRID_SIZE", Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256));
		Worker.SetDefine("DEBUG_VOXELS", Platform::GetBool("Engine.ini", "Voxels", "DebugVoxels", false));
		Worker.SetDefine("VOXEL_SCALE", static_cast<float>(Platform::GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f)));
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
	void RenderVisualization(SceneProxy& Scene, drm::CommandList& CmdList);

	inline uint32 GetVoxelGridSize() const { return VoxelGridSize; }
	inline const drm::Image& GetVoxelRadiance() const { return VoxelRadiance; }
	inline const drm::RenderPass& GetRenderPass() const { return VoxelRP; }
	inline const drm::RenderPass& GetDebugRenderPass() const { return DebugRP; }
	inline const drm::DescriptorSet& GetDescriptorSet() const { return VoxelDescriptorSet; }
	inline bool IsDebuggingEnabled() const { return DebugVoxels; }

	/** Create the render pass used for debugging. */
	void CreateDebugRenderPass(const drm::Image& SceneColor, const drm::Image& SceneDepth);

private:
	const float VoxelSize;
	const uint32 VoxelGridSize;
	const bool DebugVoxels;

	DRMDevice& Device;
	DRMShaderMap& ShaderMap;

	drm::RenderPass VoxelRP;
	drm::RenderPass DebugRP;

	drm::DescriptorSet VoxelDescriptorSet;
	drm::DescriptorSetLayout VoxelSetLayout;

	drm::Buffer WorldToVoxelBuffer;
	drm::Image VoxelBaseColor;
	drm::Image VoxelNormal;
	drm::Image VoxelRadiance;
	drm::ImageView VoxelRadianceMipMap;

	drm::Buffer VoxelPositions;
	drm::Buffer VoxelIndirectBuffer;

	struct DownsampleVolumeDescriptors
	{
		drm::DescriptorBufferInfo DownsampleVolumeUniform;
		drm::DescriptorImageInfo SrcVolume;
		drm::DescriptorImageInfo DstVolume;

		static const auto GetBindings()
		{
			static const std::vector<DescriptorBinding> Bindings =
			{
				{ 0, 1, UniformBuffer },
				{ 1, 1, SampledImage },
				{ 2, 1, StorageImage }
			};

			return Bindings;
		}
	};

	drm::Buffer DownsampleVolumeUniform;

	DescriptorSetLayout<DownsampleVolumeDescriptors> DownsampleVolumeSetLayout;

	void RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList);

	void ComputeLightInjection(SceneProxy& Scene, drm::CommandList& CmdList);

	void ComputeVolumetricDownsample(drm::CommandList& CmdList);

	void CreateVoxelRP();
};
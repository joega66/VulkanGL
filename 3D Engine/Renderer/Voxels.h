#pragma once
#include <DRM.h>
#include <ECS/Component.h>

class CameraProxy;
class EntityManager;

enum class EVoxelDebugMode
{
	None,
	Radiance,
	BaseColor,
	Normal,
};

class VoxelShader
{
public:
	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Worker.SetDefine("VOXEL_GRID_SIZE", Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256));
		Worker.SetDefine("DEBUG_VOXELS", Platform::GetBool("Engine.ini", "Voxels", "DebugVoxels", false));
	}
};

/** Voxel cone tracing lighting cache. */
class VCTLightingCache : public Component
{
public:
	VCTLightingCache(class Engine& Engine);

	/** Voxelize the scene. */
	void Render(EntityManager& ECS, CameraProxy& Camera, drm::CommandList& CmdList);
	
	/** Called just before the lighting pass. */
	void PreLightingPass(drm::CommandList& CmdList);

	/** Visualize the voxels. */
	void RenderVisualization(CameraProxy& Camera, drm::CommandList& CmdList, EVoxelDebugMode VoxelDebugMode);

	inline uint32 GetVoxelGridSize() const { return VoxelGridSize; }
	inline const drm::Image& GetVoxelRadiance() const { return VoxelRadiance; }
	inline const drm::Sampler& GetVoxelRadianceSampler() const { return VoxelRadianceSampler; }
	inline const drm::RenderPass& GetRenderPass() const { return VoxelRP; }
	inline const drm::DescriptorSet& GetDescriptorSet() const { return VoxelDescriptorSet; }
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

	drm::DescriptorSet VoxelDescriptorSet;
	drm::DescriptorSetLayout VoxelSetLayout;

	drm::Buffer VoxelUniformBuffer;
	drm::Image VoxelBaseColor;
	drm::Image VoxelNormal;
	drm::Image VoxelRadiance;

	drm::Sampler VoxelRadianceSampler;

	std::vector<drm::ImageView> VoxelRadianceMipMaps;

	drm::Buffer VoxelPositions;
	drm::Buffer VoxelIndirectBuffer;

	EImageLayout VoxelRadianceImageLayout;

	struct DownsampleVolumeDescriptors
	{
		drm::DescriptorBufferInfo DownsampleVolumeUniform;
		drm::DescriptorImageInfo SrcVolume;
		drm::DescriptorImageInfo DstVolume;

		static const auto GetBindings()
		{
			static const std::vector<DescriptorBinding> Bindings =
			{
				{ 0, 1, EDescriptorType::UniformBuffer },
				{ 1, 1, EDescriptorType::SampledImage },
				{ 2, 1, EDescriptorType::StorageImage }
			};

			return Bindings;
		}
	};

	drm::Buffer DownsampleVolumeUniform;

	DescriptorSetLayout<DownsampleVolumeDescriptors> DownsampleVolumeSetLayout;

	drm::Pipeline DownsampleVolumePipeline;

	void RenderVoxels(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeLightInjection(EntityManager& ECS, CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeVolumetricDownsample(drm::CommandList& CmdList, const drm::ImageView& SrcVolume, const drm::ImageView& DstVolume, const glm::uvec3& DstVolumeDimensions);

	void CreateVoxelRP();
};
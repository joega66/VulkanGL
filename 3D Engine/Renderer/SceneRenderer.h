#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/StaticMeshComponent.h>

struct SceneTexturesDescriptors
{
	drm::ImageView Depth;
	drm::ImageView ShadowMask;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static const std::vector<DescriptorBinding> Bindings =
		{ 
			{ 0, 1, SampledImage },
			{ 1, 1, SampledImage },
		};
		return Bindings;
	}
};

UNIFORM_STRUCT(WorldToVoxelUniform,
	glm::mat4 WorldToVoxel;
	glm::mat4 WorldToVoxelInv;
);

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

/** The Scene Renderer renders camera views and holds persistent render resources. */
class SceneRenderer
{
public:
	SceneRenderer(class Engine& Engine);

	void Render(class UserInterface& UserInterface, SceneProxy& Scene);

	DescriptorSet<CameraDescriptors> CameraDescriptorSet;
	DescriptorSet<SkyboxDescriptors> SkyboxDescriptorSet;
	DescriptorSet<SceneTexturesDescriptors> SceneTexturesDescriptorSet;
	DescriptorSet<VoxelDescriptors> VoxelDescriptorSet;

	drm::RenderPass DepthRP;
	drm::RenderPass DepthVisualizationRP;
	drm::RenderPass VoxelRP;
	drm::RenderPass VoxelVisualizationRP;
	drm::RenderPass LightingRP;
	drm::RenderPass ShadowMaskRP;

	drm::Image SceneDepth;
	drm::Image ShadowMask;
	drm::Image SceneColor;

	drm::Buffer WorldToVoxelBuffer;
	drm::Image VoxelColors;
	drm::Buffer VoxelPositions;
	drm::Buffer VoxelIndirectBuffer;

private:
	DRMDevice& Device;
	DRMShaderMap& ShaderMap;
	drm::Surface& Surface;

	void RenderDepthPrepass(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderShadowDepths(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderShadowMask(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderVoxelization(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderVoxelVisualization(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderLightingPass(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderSkybox(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList);
	void Present(drm::CommandList& CmdList);

	void CreateDepthRP();
	void CreateDepthVisualizationRP();
	void CreateVoxelRP();
	void CreateVoxelVisualizationRP();
	void CreateLightingRP();
	void CreateShadowMaskRP();

	const StaticMesh* Cube;
};
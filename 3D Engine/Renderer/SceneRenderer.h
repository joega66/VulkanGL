#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/StaticMeshComponent.h>

struct SceneTexturesDescriptors
{
	const drm::Image* Depth;
	SamplerState DepthSampler{ EFilter::Nearest };
	const drm::Image* ShadowMask;
	SamplerState ShadowMaskSampler{ EFilter::Nearest };

	static const std::vector<DescriptorTemplateEntry>& GetEntries()
	{
		static const std::vector<DescriptorTemplateEntry> Entries = 
		{ 
			{ 0, 1, SampledImage },
			{ 1, 1, SampledImage },
		};
		return Entries;
	}
};

struct VoxelDescriptors
{
	drm::BufferRef WorldToVoxelBuffer;
	const drm::Image* VoxelColors;
	drm::BufferRef VoxelPositions;
	drm::BufferRef VoxelIndirectBuffer;

	static const std::vector<DescriptorTemplateEntry>& GetEntries()
	{
		static const std::vector<DescriptorTemplateEntry> Entries =
		{
			{ 0, 1, UniformBuffer },
			{ 1, 1, StorageImage },
			{ 2, 1, StorageBuffer },
			{ 3, 1, StorageBuffer },
		};
		return Entries;
	}
};

class SceneRenderer
{
public:
	SceneRenderer(class Engine& Engine);

	void Render(class UserInterface& UserInterface, SceneProxy& Scene);

private:
	DRMDevice& Device;

	DRMShaderMap& ShaderMap;

	drm::Surface& Surface;

	drm::RenderPass DepthRP;

	drm::RenderPass DepthVisualizationRP;

	drm::RenderPass VoxelRP;

	drm::RenderPass VoxelVisualizationRP;

	drm::RenderPass LightingRP;

	drm::RenderPass ShadowMaskRP;

	drm::Image SceneDepth;

	drm::Image ShadowMask;

	DescriptorSet<SceneTexturesDescriptors> SceneTextures;

	drm::Image VoxelColors;

	DescriptorSet<VoxelDescriptors> VoxelDescriptorSet;

	drm::Image SceneColor;

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
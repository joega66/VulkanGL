#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/StaticMeshComponent.h>

struct SceneTexturesDescriptors
{
	drm::ImageRef Depth;
	SamplerState DepthSampler{ EFilter::Nearest };
	drm::ImageRef ShadowMask;
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
	drm::ImageRef VoxelColors;
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
	SceneRenderer(DRMDevice& Device, drm::Surface& Surface, class Scene& Scene, class Screen& Screen);

	void Render(class UserInterface& UserInterface, SceneProxy& Scene);

private:
	DRMDevice& Device;

	drm::Surface& Surface;

	const StaticMesh* Cube;

	drm::RenderPass DepthRP;

	drm::RenderPass DepthVisualizationRP;

	drm::RenderPass VoxelRP;

	drm::RenderPass VoxelVisualizationRP;

	drm::RenderPass LightingRP;

	drm::RenderPass ShadowMaskRP;

	drm::ImageRef ShadowMask;

	DescriptorSet<SceneTexturesDescriptors> SceneTextures;

	DescriptorSet<VoxelDescriptors> VoxelDescriptorSet;

	drm::ImageRef SceneColor;

	void RenderDepthPrepass(SceneProxy& Scene, drm::CommandList& CmdList);

	void RenderShadowDepths(SceneProxy& Scene, drm::CommandList& CmdList);

	void RenderShadowMask(SceneProxy& Scene, drm::CommandList& CmdList);

	void RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList);

	void RenderVoxelization(SceneProxy& Scene, drm::CommandList& CmdList);

	void RenderVoxelVisualization(SceneProxy& Scene, drm::CommandList& CmdList);

	void RenderLightingPass(SceneProxy& Scene, drm::CommandList& CmdList);

	void RenderSkybox(SceneProxy& Scene, drm::CommandList& CmdList);

	void RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList);

	void Present(drm::CommandListRef CmdList);

	void CreateDepthRP();

	void CreateDepthVisualizationRP();

	void CreateVoxelRP();

	void CreateVoxelVisualizationRP();

	void CreateLightingRP();

	void CreateShadowMaskRP();
};
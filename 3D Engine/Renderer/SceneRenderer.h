#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/StaticMeshComponent.h>

class SceneRenderer
{
public:
	SceneRenderer(DRMDevice& Device, drm::Surface& Surface, class Scene& Scene, class Screen& Screen);

	void Render(SceneProxy& Scene);

private:
	DRMDevice& Device;

	drm::Surface& Surface;

	const StaticMesh* Cube;

	drm::RenderPassRef DepthRP;

	drm::RenderPassRef DepthVisualizationRP;

	drm::RenderPassRef VoxelRP;

	drm::RenderPassRef VoxelVisualizationRP;

	drm::RenderPassRef LightingRP;

	drm::RenderPassRef ShadowMaskRP;

	drm::DescriptorSetRef SceneTextures;

	drm::ImageRef SceneColor;

	drm::ImageRef SceneDepth;

	drm::ImageRef ShadowMask;

	drm::DescriptorSetRef VoxelsDescriptorSet;

	drm::BufferRef VoxelIndirectBuffer;

	drm::ImageRef VoxelColors;

	drm::BufferRef VoxelPositions;

	drm::BufferRef WorldToVoxelBuffer;

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
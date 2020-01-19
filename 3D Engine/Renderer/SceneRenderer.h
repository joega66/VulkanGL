#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/StaticMeshComponent.h>

class SceneRenderer
{
public:
	SceneRenderer(DRM& Device, drm::Surface& Surface, class Scene& Scene, class Screen& Screen);

	void Render(SceneProxy& Scene);

private:
	DRM& Device;

	drm::Surface& Surface;

	uint32 ImageIndex;

	const StaticMesh* Cube;

	drm::RenderPassRef DepthRenderPass;

	drm::DescriptorSetRef SceneTextures;

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
};
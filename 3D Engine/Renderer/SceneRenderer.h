#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/StaticMeshComponent.h>

extern uint32 gVoxelGridSize;

class SceneRenderer
{
public:
	SceneRenderer(DRM& Device, class Scene& Scene, class Screen& Screen);

	void Render(SceneProxy& Scene);

private:
	DRM& Device;

	const StaticMesh* Cube;

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
#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/CStaticMesh.h>

class Scene;
class RenderCommandList;

class SceneRenderer
{
public:
	SceneRenderer(class Scene& Scene);

	void Render(SceneProxy& Scene);

private:
	StaticMeshRef Cube;

	drm::DescriptorSetRef SceneTextures;

	drm::ImageRef SceneDepth;

	drm::ImageRef ShadowMask;

	drm::DescriptorSetRef VoxelsDescriptorSet;

	drm::BufferRef VoxelIndirectBuffer;

	drm::ImageRef VoxelColors;

	drm::BufferRef VoxelPositions;

	drm::BufferRef WorldToVoxelBuffer;

	void RenderDepthPrepass(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderShadowDepths(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderShadowMask(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderVoxels(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderVoxelization(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderVoxelVisualization(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderLightingPass(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderSkybox(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderDepthVisualization(SceneProxy& Scene, RenderCommandList& CmdList);
};
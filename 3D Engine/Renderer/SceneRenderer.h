#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/CStaticMesh.h>

class Scene;
class RenderCommandList;

class SceneRenderer
{
public:
	SceneRenderer(const class Scene& Scene);

	void Render(SceneProxy& Scene);

private:
	StaticMeshRef Cube;

	// Scene render targets.
	drm::ImageRef SceneDepth;

	//drm::ImageRef VoxelColor;

	drm::BufferRef VoxelColors;

	drm::BufferRef VoxelPositions;

	drm::BufferRef VoxelOrthoProjBuffer;

	drm::DescriptorSetRef VoxelsDescriptorSet;

	void RenderVoxels(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderVoxelization(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderVoxelVisualization(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderRayMarching(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderLightingPass(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderSkybox(SceneProxy& Scene, RenderCommandList& CmdList);
};
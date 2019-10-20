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

	drm::UniformBufferRef VoxelOrthoProjBuffer;

	drm::DescriptorSetRef VoxelsDescriptorSet;

	drm::ImageRef Voxels;

	void RenderVoxels(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderRayMarching(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderLightingPass(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderSkybox(SceneProxy& Scene, RenderCommandList& CmdList);
};
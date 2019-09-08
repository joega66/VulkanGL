#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>

class Scene;
class RenderCommandList;

class SceneRenderer
{
public:
	SceneRenderer();

	void Render(SceneProxy& Scene);

private:
	// Scene render targets.
	drm::ImageRef SceneDepth;
	drm::ImageRef OutlineDepthStencil;

	void RenderRayMarching(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderLightingPass(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderLines(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderSkybox(SceneProxy& Scene, RenderCommandList& CmdList);

	void RenderOutlines(SceneProxy& Scene, RenderCommandList& CmdList);
};
#pragma once
#include <DRM.h>

class Scene;
class RenderCommandList;

class SceneRenderer
{
public:
	SceneRenderer();

	void Render(Scene& Scene);

	void SetResources(RenderCommandList& CmdList, const drm::ShaderRef& Shader, const class SceneBindings& Bindings) const;

private:
	drm::UniformBufferRef ViewUniform;
	drm::StorageBufferRef PointLightBuffer;
	drm::ImageRef SceneDepth;
	drm::ImageRef OutlineDepthStencil;

	void InitView(Scene& Scene);

	void RenderRayMarching(Scene& Scene, RenderCommandList& CmdList);

	void RenderLightingPass(Scene& Scene, RenderCommandList& CmdList);

	void RenderLines(Scene& Scene, RenderCommandList& CmdList);

	void RenderSkybox(Scene& Scene, RenderCommandList& CmdList);

	void RenderOutlines(Scene& Scene, RenderCommandList& CmdList);
};
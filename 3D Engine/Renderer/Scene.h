#pragma once
#include "LightingPass.h"
#include "EditorPrimitives.h"

class Scene
{
public:
	View View;

	DrawingPlanList<LightingPassDrawingPlan> LightingPass;
	DrawingPlanList<DepthPassDrawingPlan> Stencil;
	DrawingPlanList<OutlineDrawingPlan> Outline;
	DrawingPlanList<LineDrawingPlan> Lines;

	drm::StorageBufferRef PointLightBuffer;

	drm::ImageRef Skybox;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Render();

	static Scene& Get();

	void SetResources(RenderCommandList& CmdList, const drm::ShaderRef& Shader, const class SceneBindings& Bindings) const;

private:
	Scene();

	drm::ImageRef SceneDepth;
	drm::ImageRef OutlineDepthStencil;

	void RenderRayMarching(RenderCommandList& CmdList);

	void RenderLightingPass(RenderCommandList& CmdList);

	void RenderLines(RenderCommandList& CmdList);

	void RenderSkybox(RenderCommandList& CmdList);

	void RenderOutlines(RenderCommandList& CmdList);
};
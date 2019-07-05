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

	drm::StorageBufferRef LightBuffer;

	drm::ImageRef Skybox;

	Scene();
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Render();
	static Scene& Get();

private:
	drm::ImageRef SceneDepth;
	drm::ImageRef OutlineDepthStencil;

	void RenderVoxels(RenderCommandList& CmdList);
	void RenderRayMarching(RenderCommandList& CmdList);
	void RenderLightingPass(RenderCommandList& CmdList);
	void RenderEditorPrimitives(RenderCommandList& CmdList);

	void RenderLines(RenderCommandList& CmdList);
	void RenderSkybox(RenderCommandList& CmdList);
	void RenderOutlines(RenderCommandList& CmdList);
};
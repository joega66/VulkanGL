#pragma once
#include "LightingPass.h"
#include "EditorPrimitives.h"

class Scene
{
public:
	View View;

	DrawingPlanList<LightingPassDrawingPlan> LightingPassDrawingPlans;
	DrawingPlanList<DepthPassDrawingPlan> Stencil;
	DrawingPlanList<OutlineDrawingPlan> Outline;
	DrawingPlanList<LineDrawingPlan> Lines;

	GLStorageBufferRef LightBuffer;

	GLImageRef Skybox;

	Scene();
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Render();
	static Scene& Get();

private:
	GLImageRef SceneDepth;
	GLImageRef OutlineDepthStencil;

	void RenderRayMarching(RenderCommandList& CmdList);
	void RenderLightingPass(RenderCommandList& CmdList);
	void RenderEditorPrimitives(RenderCommandList& CmdList);

	void RenderLines(RenderCommandList& CmdList);
	void RenderSkybox(RenderCommandList& CmdList);
	void RenderOutlines(RenderCommandList& CmdList);
};
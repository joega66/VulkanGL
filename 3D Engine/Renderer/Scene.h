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

	GLStorageBufferRef LightBuffer;

	GLImageRef Skybox;

	DrawingPlanList<LineDrawingPlan> Lines;

	Scene();
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Render();
	static Scene& Get();

private:
	GLImageRef SceneDepth;
	GLImageRef OutlineDepthStencil;

	void RenderRayMarching();
	void RenderLightingPass();
	void RenderEditorPrimitives();
	void RenderLines();
	void RenderSkybox();
	void RenderOutlines();
};
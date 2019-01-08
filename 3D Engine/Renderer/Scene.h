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

	GLImageRef Skybox;

	void DrawLine(const DrawLineInfo& DrawLineInfo);

	Scene();
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Render();
	static Scene& Get();

private:
	GLImageRef SceneDepth;
	GLImageRef OutlineDepthStencil;

	std::vector<DrawLineInfo> Lines;

	void RenderLightingPass();
	void RenderEditorPrimitives();
	void RenderLines();
	void RenderSkybox();
	void RenderOutlines();
};
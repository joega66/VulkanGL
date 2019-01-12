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

	void DrawLine(Entity Entity, const DrawLineInfo& DrawLineInfo);
	void RemoveLine(Entity Entity);

	Scene();
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Render();
	static Scene& Get();

private:
	GLImageRef SceneDepth;
	GLImageRef OutlineDepthStencil;

	// @todo Lines are perfect for drawing plans...
	std::multimap<uint64, DrawLineInfo> Lines;

	void RenderLightingPass();
	void RenderEditorPrimitives();
	void RenderLines();
	void RenderSkybox();
	void RenderOutlines();
};
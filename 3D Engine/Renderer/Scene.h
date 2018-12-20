#pragma once
#include "LightingPass.h"
#include "EditorPrimitives.h"

class Scene
{
public:
	View View;

	DrawingPlanList<LightingPassDrawingPlan> LightingPassDrawingPlans;

	struct
	{
		DrawingPlanList<DepthPassDrawingPlan> Prepass;
		DrawingPlanList<ObjectHighlightDrawingPlan> Final;
	} Highlighted;
	
	Scene();
	void Render();
	void RenderLightingPass();
	void RenderEditorPrimitives();

private:
	GLImageRef SceneDepth;
	GLRenderTargetViewRef SceneDepthView;

	GLImageRef HighlightDepthStencil;
	GLRenderTargetViewRef HighlightDepthStencilView;
};
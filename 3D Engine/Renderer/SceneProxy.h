#pragma once
#include "Light.h"
#include "LightingPass.h"
#include "EditorPrimitives.h"

class SceneProxy
{
	friend class CoreEngine;
	SceneProxy(Scene& Scene);

public:
	SceneProxy(const SceneProxy&) = delete;
	SceneProxy& operator=(const SceneProxy&) = delete;

	const drm::ImageRef Skybox;

	DrawList<LightingPassDrawPlan> LightingPass;

	DrawList<DepthPassDrawPlan> Stencil;

	DrawList<OutlineDrawPlan> Outline;

	DrawList<LineDrawPlan> Lines;

	std::vector<PointLightProxy> PointLightProxies;

	drm::UniformBufferRef ViewUniform;

	drm::StorageBufferRef PointLightBuffer;

	void SetResources(RenderCommandList& CmdList, const drm::ShaderRef& Shader, const class SceneBindings& Bindings) const;

private:

	void InitView(Scene& Scene);
	void InitLights(Scene& Scene);
	void InitDrawLists(Scene& Scene);
	void InitLightingPassDrawList(Scene& Scene);
	void InitOutlineDrawList(Scene& Scene);
};
#pragma once
#include "Light.h"
#include "LightingPass.h"
#include "EditorPrimitives.h"
#include "DrawingPlan.h"

namespace EStaticDrawListType
{
	enum EStaticDrawListType
	{
		Opaque,
		Masked,
		Max
	};
};

class SceneProxy
{
	friend class CoreEngine;
	SceneProxy(Scene& Scene);

	static constexpr uint32 VIEW_BINDING = 0;
	static constexpr uint32 DIRECTIONAL_LIGHT_BINDING = 1;
	static constexpr uint32 POINT_LIGHT_BINDING = 2;

public:
	SceneProxy(const SceneProxy&) = delete;
	SceneProxy& operator=(const SceneProxy&) = delete;

	const drm::ImageRef Skybox;

	std::array<DrawList<LightingPassDrawPlan>, EStaticDrawListType::Max> LightingPass;

	std::vector<DirectionalLightProxy> DirectionalLightProxies;

	std::vector<PointLightProxy> PointLightProxies;

	drm::UniformBufferRef ViewUniform;

	drm::StorageBufferRef DirectionalLightBuffer;

	drm::StorageBufferRef PointLightBuffer;

	drm::DescriptorSetRef DescriptorSet;

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Worker.SetDefine("SCENE_SET", 0);
		Worker.SetDefine("VIEW_BINDING", VIEW_BINDING);
		Worker.SetDefine("DIRECTIONAL_LIGHT_BINDING", DIRECTIONAL_LIGHT_BINDING);
		Worker.SetDefine("POINT_LIGHT_BINDING", POINT_LIGHT_BINDING);
	}

private:
	void InitView(Scene& Scene);
	void InitLights(Scene& Scene);
	void InitDrawLists(Scene& Scene);
	void InitLightingPassDrawList(Scene& Scene);
};
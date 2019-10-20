#pragma once
#include <Engine/Scene.h>
#include "Light.h"
#include "LightingPass.h"
#include "Voxels.h"
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

public:
	SceneProxy(const SceneProxy&) = delete;
	SceneProxy& operator=(const SceneProxy&) = delete;

	const drm::ImageRef Skybox;

	std::array<DrawList<LightingPassDrawPlan>, EStaticDrawListType::Max> LightingPass;

	DrawList<VoxelizationPass> VoxelsPass;

	std::vector<DirectionalLightProxy> DirectionalLightProxies;

	std::vector<PointLightProxy> PointLightProxies;

	drm::UniformBufferRef ViewUniform;

	drm::StorageBufferRef DirectionalLightBuffer;

	drm::StorageBufferRef PointLightBuffer;

	drm::DescriptorSetRef DescriptorSet;

	drm::DescriptorSetRef VoxelsDescriptorSet;

private:
	void InitView(Scene& Scene);
	void InitLights(Scene& Scene);
	void InitDrawLists(Scene& Scene);
	void InitLightingPassDrawList(Scene& Scene);
};
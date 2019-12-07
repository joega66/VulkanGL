#pragma once
#include <Engine/Scene.h>
#include "Light.h"
#include "DepthPrepass.h"
#include "LightingPass.h"
#include "Voxels.h"
#include "MeshDrawInterface.h"

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
	const View& View;

	SceneProxy(const SceneProxy&) = delete;
	SceneProxy& operator=(const SceneProxy&) = delete;

	const drm::ImageRef Skybox;

	MeshDrawInterface<DepthPrepass> DepthPrepass;

	std::array<MeshDrawInterface<LightingPass>, EStaticDrawListType::Max> LightingPass;

	MeshDrawInterface<VoxelizationPass> VoxelsPass;

	std::vector<DirectionalLightProxy> DirectionalLightProxies;

	std::vector<PointLightProxy> PointLightProxies;

	drm::BufferRef ViewUniform;

	drm::BufferRef DirectionalLightBuffer;

	drm::BufferRef PointLightBuffer;

	drm::DescriptorSetRef DescriptorSet;

private:
	void InitView(Scene& Scene);
	void InitLights(Scene& Scene);
	void InitDrawLists(Scene& Scene);
	static void AddToDrawLists(SceneProxy& Scene, const MeshProxyRef& MeshProxy);
};
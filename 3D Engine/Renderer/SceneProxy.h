#pragma once
#include <Engine/Scene.h>
#include "Light.h"
#include "DepthPrepass.h"
#include "LightingPass.h"
#include "Voxels.h"
#include "ShadowDepthPass.h"
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
	EntityManager& ECS;

	const View& View;

	SceneProxy(const SceneProxy&) = delete;
	SceneProxy& operator=(const SceneProxy&) = delete;

	const drm::ImageRef Skybox;

	MeshDrawInterface<DepthPrepass> DepthPrepass;

	std::array<MeshDrawInterface<LightingPass>, EStaticDrawListType::Max> LightingPass;

	MeshDrawInterface<VoxelizationPass> VoxelsPass;

	MeshDrawInterface<ShadowDepthPass> ShadowDepthPass;

	drm::BufferRef ViewUniform;

	drm::BufferRef DirectionalLightBuffer;

	drm::BufferRef PointLightBuffer;

	drm::DescriptorSetRef DescriptorSet;

private:
	void InitView();
	void InitLights();
	void InitDrawLists();
	void AddToDrawLists(const MeshProxy& MeshProxy);

	std::vector<MeshProxy> MeshProxies;
};
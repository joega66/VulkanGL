#pragma once
#include <Engine/Scene.h>
#include "MeshDrawCommand.h"

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

	drm::ImageRef Skybox;

	drm::BufferRef ViewUniform;

	drm::BufferRef DirectionalLightBuffer;

	drm::BufferRef PointLightBuffer;

	drm::DescriptorSetRef DescriptorSet;

	std::vector<MeshProxy> MeshProxies;

	std::vector<MeshDrawCommand> DepthPrepass;

	std::vector<MeshDrawCommand> ShadowDepthPass;

	std::vector<MeshDrawCommand> VoxelsPass;

	std::array<std::vector<MeshDrawCommand>, EStaticDrawListType::Max> LightingPass;

private:
	void InitView();
	void InitLights();
	void InitDirectionalLights();
	void InitPointLights();
	void InitDrawLists();

	void AddToDrawLists(const MeshProxy& MeshProxy);
	void AddToDepthPrepass(const MeshProxy& MeshProxy);
	void AddToShadowDepthPass(const MeshProxy& MeshProxy);
	void AddToVoxelsPass(const MeshProxy& MeshProxy);
	void AddToLightingPass(const MeshProxy& MeshProxy);
};
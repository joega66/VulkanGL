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
	DRMShaderMap& ShaderMap;

	EntityManager& ECS;

	const View& View;

	SceneProxy(const SceneProxy&) = delete;
	SceneProxy& operator=(const SceneProxy&) = delete;

	/** Skybox to be rendered in the scene. */
	drm::ImageRef Skybox;

	drm::BufferRef ViewUniform;

	drm::BufferRef DirectionalLightBuffer;

	drm::BufferRef PointLightBuffer;

	drm::DescriptorSetRef DescriptorSet;

	/** Meshes in the scene. */
	std::vector<MeshProxy> MeshProxies;

	/** Meshes that passed view frustum culling. */
	std::vector<const MeshProxy*> VisibleMeshProxies;

	std::vector<MeshDrawCommand> DepthPrepass;

	std::vector<MeshDrawCommand> ShadowDepthPass;

	std::vector<MeshDrawCommand> VoxelsPass;

	std::array<std::vector<MeshDrawCommand>, EStaticDrawListType::Max> LightingPass;

private:
	void InitView();
	void InitLights();
	void InitDirectionalLights();
	void InitPointLights();
	void InitMeshDrawCommands();

	void AddToDepthPrepass(const MeshProxy& MeshProxy);
	void AddToShadowDepthPass(const MeshProxy& MeshProxy);
	void AddToVoxelsPass(const MeshProxy& MeshProxy);
	void AddToLightingPass(const MeshProxy& MeshProxy);
};
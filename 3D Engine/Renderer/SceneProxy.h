#pragma once
#include <Engine/Camera.h>
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

struct CameraDescriptors
{
	const drm::Buffer* CameraUniform;
	const drm::Buffer* DirectionalLightBuffer;
	const drm::Buffer* PointLightBuffer;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, UniformBuffer },
			{ 1, 1, StorageBuffer },
			{ 2, 1, StorageBuffer },
		};
		return Bindings;
	}
};

struct SkyboxDescriptors
{
	const drm::Image* Skybox;
	const drm::Sampler* Sampler;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, SampledImage }
		};
		return Bindings;
	}
};

class Engine;

class SceneProxy : public Camera
{
	friend class Engine;
	SceneProxy(Engine& Engine, class SceneRenderer& SceneRenderer);

public:
	SceneProxy(const SceneProxy&) = delete;
	SceneProxy& operator=(const SceneProxy&) = delete;

	class EntityManager& ECS;

	drm::Buffer CameraUniform;
	drm::Buffer DirectionalLightBuffer;
	drm::Buffer PointLightBuffer;

	DescriptorSet<SkyboxDescriptors> SkyboxDescriptorSet;

	DescriptorSet<CameraDescriptors> CameraDescriptorSet;

	std::vector<MeshDrawCommand> DepthPrepass;

	std::vector<MeshDrawCommand> ShadowDepthPass;

	std::vector<MeshDrawCommand> VoxelsPass;

	std::array<std::vector<MeshDrawCommand>, EStaticDrawListType::Max> LightingPass;

private:
	void InitView(Engine& Engine);
	void InitLights(Engine& Engine);
	void InitDirectionalLights(Engine& Engine);
	void InitPointLights(Engine& Engine);
	void InitMeshDrawCommands(Engine& Engine, SceneRenderer& SceneRenderer);

	void AddToDepthPrepass(SceneRenderer& SceneRenderer, DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy);
	void AddToVoxelsPass(SceneRenderer& SceneRenderer, DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy);
	void AddToLightingPass(SceneRenderer& SceneRenderer, DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy);
};
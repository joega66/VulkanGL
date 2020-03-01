#pragma once
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

class Engine;

class SceneProxy
{
	friend class Engine;

	SceneProxy(Engine& Engine);

public:
	SceneProxy(const SceneProxy&) = delete;
	SceneProxy& operator=(const SceneProxy&) = delete;

	class EntityManager& ECS;

	drm::Buffer CameraUniform;
	drm::Buffer DirectionalLightBuffer;
	drm::Buffer PointLightBuffer;

	std::vector<MeshDrawCommand> DepthPrepass;
	std::vector<MeshDrawCommand> VoxelsPass;
	std::array<std::vector<MeshDrawCommand>, EStaticDrawListType::Max> LightingPass;

private:
	void InitView(Engine& Engine);
	void InitLights(Engine& Engine);
	void InitDirectionalLights(Engine& Engine);
	void InitPointLights(Engine& Engine);
	void InitMeshDrawCommands(Engine& Engine);

	void AddToDepthPrepass(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy);
	void AddToVoxelsPass(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy);
	void AddToLightingPass(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy);
};
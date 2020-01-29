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

class SceneProxy : public Camera
{
	friend class EngineMain;
	SceneProxy(DRMDevice& Device, Scene& Scene);

public:
	SceneProxy(const SceneProxy&) = delete;
	SceneProxy& operator=(const SceneProxy&) = delete;

	DRMShaderMap& ShaderMap;

	EntityManager& ECS;

	/** Skybox to be rendered in the scene. */
	drm::ImageRef Skybox;

	drm::BufferRef CameraUniform;

	drm::BufferRef DirectionalLightBuffer;

	drm::BufferRef PointLightBuffer;

	drm::DescriptorSetRef DescriptorSet;

	std::vector<MeshDrawCommand> DepthPrepass;

	std::vector<MeshDrawCommand> ShadowDepthPass;

	std::vector<MeshDrawCommand> VoxelsPass;

	std::array<std::vector<MeshDrawCommand>, EStaticDrawListType::Max> LightingPass;

private:
	void InitView(DRMDevice& Device);
	void InitLights(DRMDevice& Device);
	void InitDirectionalLights(DRMDevice& Device);
	void InitPointLights(DRMDevice& Device);
	void InitMeshDrawCommands(DRMDevice& Device);

	void AddToDepthPrepass(const MeshProxy& MeshProxy);
	void AddToShadowDepthPass(const MeshProxy& MeshProxy);
	void AddToVoxelsPass(const MeshProxy& MeshProxy);
	void AddToLightingPass(const MeshProxy& MeshProxy);
};
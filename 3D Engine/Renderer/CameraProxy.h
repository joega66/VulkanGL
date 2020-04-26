#pragma once
#include "MeshDrawCommand.h"

class Engine;

struct CameraDescriptors
{
	drm::DescriptorBufferInfo CameraUniform;
	drm::DescriptorImageInfo SceneDepth;
	drm::DescriptorImageInfo GBuffer0;
	drm::DescriptorImageInfo GBuffer1;
	drm::DescriptorImageInfo RadianceVolume;
	drm::DescriptorImageInfo SceneColor;

	static auto& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, EDescriptorType::UniformBuffer },
			{ 1, 1, EDescriptorType::SampledImage },
			{ 2, 1, EDescriptorType::SampledImage },
			{ 3, 1, EDescriptorType::SampledImage },
			{ 4, 1, EDescriptorType::SampledImage },
			{ 5, 1, EDescriptorType::StorageImage },
		};
		return Bindings;
	}
};

/** The camera proxy holds camera-dependent render data. */
class CameraProxy
{
	friend class Engine;
	CameraProxy(Engine& Engine);

public:
	CameraProxy(const CameraProxy&) = delete;
	CameraProxy& operator=(const CameraProxy&) = delete;

	drm::RenderPass SceneRP;
	drm::RenderPass GBufferRP;
	std::vector<drm::RenderPass> UserInterfaceRP;
	
	drm::Buffer CameraUniformBuffer;

	drm::Image SceneDepth;
	drm::Image SceneColor;
	drm::Image GBuffer0;
	drm::Image GBuffer1;

	DescriptorSet<CameraDescriptors> CameraDescriptorSet;

	std::vector<MeshDrawCommand> GBufferPass;
	std::vector<MeshDrawCommand> VoxelsPass;

	void Update(Engine& Engine);

private:
	void UpdateCameraUniform(Engine& Engine);
	void BuildMeshDrawCommands(Engine& Engine);

	void AddToGBufferPass(Engine& Engine, const MeshProxy& MeshProxy);
	void AddToVoxelsPass(Engine& Engine, const MeshProxy& MeshProxy);

	void CreateSceneRP(DRMDevice& Device);
	void CreateGBufferRP(DRMDevice& Device);
	void CreateUserInterfaceRP(DRMDevice& Device, drm::Surface& Surface);
};
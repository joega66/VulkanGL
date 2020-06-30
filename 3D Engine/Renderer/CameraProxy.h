#pragma once
#include "MeshDrawCommand.h"

class Engine;

struct CameraDescriptors
{
	gpu::DescriptorBufferInfo CameraUniform;
	gpu::DescriptorImageInfo SceneDepth;
	gpu::DescriptorImageInfo GBuffer0;
	gpu::DescriptorImageInfo GBuffer1;
	gpu::DescriptorImageInfo SceneColor;
	gpu::DescriptorImageInfo _SSGIHistory;

	static auto& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, EDescriptorType::UniformBuffer },
			{ 1, 1, EDescriptorType::SampledImage },
			{ 2, 1, EDescriptorType::SampledImage },
			{ 3, 1, EDescriptorType::SampledImage },
			{ 4, 1, EDescriptorType::StorageImage },
			{ 5, 1, EDescriptorType::StorageImage },
		};
		return Bindings;
	}
};

class CameraProxy
{
	friend class Engine;
	CameraProxy(Engine& Engine);

public:
	CameraProxy(const CameraProxy&) = delete;
	CameraProxy& operator=(const CameraProxy&) = delete;

	gpu::RenderPass SceneRP;
	gpu::RenderPass GBufferRP;
	std::vector<gpu::RenderPass> UserInterfaceRP;
	
	gpu::Buffer CameraUniformBuffer;

	gpu::Image SceneDepth;
	gpu::Image SceneColor;
	gpu::Image GBuffer0;
	gpu::Image GBuffer1;
	gpu::Image _SSGIHistory;

	DescriptorSet<CameraDescriptors> CameraDescriptorSet;

	std::vector<MeshDrawCommand> GBufferPass;

	void Update(Engine& Engine);

private:
	void UpdateCameraUniform(Engine& Engine);

	void BuildMeshDrawCommands(Engine& Engine);

	void AddToGBufferPass(Engine& Engine, const MeshProxy& MeshProxy);

	void CreateSceneRP(gpu::Device& Device);
	void CreateGBufferRP(gpu::Device& Device);
	void CreateUserInterfaceRP(gpu::Device& Device, gpu::Surface& Surface);
};
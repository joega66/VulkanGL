#pragma once
#include "MeshDrawCommand.h"

class Engine;

struct CameraDescriptors
{
	gpu::DescriptorBufferInfo _CameraUniform;
	gpu::DescriptorImageInfo _SceneDepth;
	gpu::DescriptorImageInfo _GBuffer0;
	gpu::DescriptorImageInfo _GBuffer1;
	gpu::DescriptorImageInfo _SceneColor;
	gpu::DescriptorImageInfo _SSGIHistory;

	static auto& GetBindings()
	{
		static std::vector<DescriptorBinding> bindings =
		{
			{ 0, 1, EDescriptorType::UniformBuffer },
			{ 1, 1, EDescriptorType::SampledImage },
			{ 2, 1, EDescriptorType::SampledImage },
			{ 3, 1, EDescriptorType::SampledImage },
			{ 4, 1, EDescriptorType::StorageImage },
			{ 5, 1, EDescriptorType::StorageImage },
		};
		return bindings;
	}
};

class CameraProxy
{
	friend class Engine;
	CameraProxy(Engine& engine);

public:
	CameraProxy(const CameraProxy&) = delete;
	CameraProxy& operator=(const CameraProxy&) = delete;

	gpu::RenderPass _SceneRP;
	gpu::RenderPass _GBufferRP;
	std::vector<gpu::RenderPass> _UserInterfaceRP;
	
	gpu::Buffer _CameraUniformBuffer;

	gpu::Image _SceneDepth;
	gpu::Image _SceneColor;
	gpu::Image _GBuffer0;
	gpu::Image _GBuffer1;
	gpu::Image _SSGIHistory;

	DescriptorSet<CameraDescriptors> _CameraDescriptorSet;

	std::vector<MeshDrawCommand> _GBufferPass;

	void Update(Engine& engine);

private:
	void UpdateCameraUniform(Engine& engine);

	void BuildMeshDrawCommands(Engine& engine);

	void AddToGBufferPass(Engine& engine, const MeshProxy& meshProxy);

	void CreateSceneRP(gpu::Device& device);
	void CreateGBufferRP(gpu::Device& device);
	void CreateUserInterfaceRP(gpu::Device& device, gpu::Surface& surface);
};
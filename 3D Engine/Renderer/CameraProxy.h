#pragma once
#include "MeshDrawCommand.h"

class Engine;

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

	gpu::DescriptorSet _CameraDescriptorSet;

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
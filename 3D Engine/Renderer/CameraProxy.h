#pragma once
#include <ECS/Component.h>
#include <GPU/GPU.h>

class Screen;
class Camera;

class CameraProxy : public Component
{
public:
	CameraProxy(Screen& screen, gpu::Device& device, gpu::Compositor& compositor);

	gpu::RenderPass _SkyboxRP;
	gpu::RenderPass _GBufferRP;
	std::vector<gpu::RenderPass> _UserInterfaceRP;
	
	gpu::Buffer _CameraUniformBuffer;

	gpu::Image _SceneDepth;
	gpu::Image _SceneColor;
	gpu::Image _GBuffer0;
	gpu::Image _GBuffer1;
	gpu::Image _SSRHistory;
	gpu::Image _SSGIHistory;
	gpu::Image _DirectLighting;

	gpu::DescriptorSet _CameraDescriptorSet;

	void Update(const Camera& camera);

private:
	void UpdateCameraUniform(const Camera& camera);

	void CreateGBufferRP(gpu::Device& device);
	void CreateSkyboxRP(gpu::Device& device);
	void CreateUserInterfaceRP(gpu::Device& device, gpu::Compositor& swapchain);
};
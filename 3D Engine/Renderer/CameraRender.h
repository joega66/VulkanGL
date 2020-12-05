#pragma once
#include <ECS/Component.h>
#include <GPU/GPU.h>

class CameraRender : public Component
{
public:
	CameraRender() = default;

	gpu::RenderPass _GBufferRP;
	gpu::RenderPass _SkyboxRP;

	gpu::Image _SceneDepth;
	gpu::Image _SceneColor;
	gpu::Image _GBuffer0;
	gpu::Image _GBuffer1;
	gpu::Image _SSRHistory;
	gpu::Image _SSGIHistory;
	gpu::Image _DirectLighting;

	void Resize(gpu::Device& device, uint32 width, uint32 height);

	void SetDynamicOffset(uint32 dynamicOffset) { _DynamicOffset = dynamicOffset; }
	inline uint32 GetDynamicOffset() const { return _DynamicOffset; }

private:
	void CreateGBufferRP(gpu::Device& device);

	void CreateSkyboxRP(gpu::Device& device);

	uint32 _DynamicOffset;
};
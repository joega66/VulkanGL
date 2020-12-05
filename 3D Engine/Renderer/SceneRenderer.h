#pragma once
#include <GPU/GPU.h>
#include <Engine/Screen.h>
#include "CameraProxy.h"

class Engine;
class Camera;
class EntityManager;
class AssetManager;

class SceneRenderer
{
public:
	SceneRenderer(Engine& engine);

	void Render();

private:
	gpu::Device& _Device;
	gpu::ShaderLibrary& _ShaderLibrary;
	gpu::Compositor& _Compositor;
	EntityManager& _ECS;
	AssetManager& _Assets;

	std::shared_ptr<ScreenResizeEvent> _ScreenResizeEvent;

	std::vector<gpu::RenderPass> _UserInterfaceRP;

	gpu::Semaphore _AcquireNextImageSem;
	gpu::Semaphore _EndOfFrameSem;

	void RenderGBufferPass(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandBuffer& cmdBuf);
	void RenderShadowDepths(CameraProxy& camera, gpu::CommandBuffer& cmdBuf);
	void ComputeLightingPass(CameraProxy& camera, gpu::CommandBuffer& cmdBuf);
	void ComputeDeferredLight(CameraProxy& camera, gpu::CommandBuffer& cmdBuf, const struct LightingParams& light, bool isFirstLight);
	void ComputeSSGI(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandBuffer& cmdBuf);
	void ComputeRayTracing(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandBuffer& cmdBuf);
	void RenderSkybox(CameraProxy& camera, gpu::CommandBuffer& cmdBuf);
	void ComputePostProcessing(const gpu::Image& displayImage, CameraProxy& camera, gpu::CommandBuffer& cmdBuf);
	void RenderUserInterface(gpu::CommandBuffer& cmdBuf, const gpu::RenderPass& renderPass);

	void CreateUserInterfacePipeline();
};
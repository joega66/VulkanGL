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

	void RenderGBufferPass(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandList& cmdList);
	void RenderShadowDepths(CameraProxy& camera, gpu::CommandList& cmdList);
	void ComputeLightingPass(CameraProxy& camera, gpu::CommandList& cmdList);
	void ComputeDeferredLight(CameraProxy& camera, gpu::CommandList& cmdList, const struct LightingParams& light, bool isFirstLight);
	void ComputeSSGI(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandList& cmdList);
	void ComputeRayTracing(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandList& cmdList);
	void RenderSkybox(CameraProxy& camera, gpu::CommandList& cmdList);
	void ComputePostProcessing(const gpu::Image& displayImage, CameraProxy& camera, gpu::CommandList& cmdList);
};
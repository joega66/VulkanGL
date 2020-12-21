#pragma once
#include <GPU/GPU.h>
#include <Engine/Screen.h>
#include "CameraRender.h"

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
	gpu::Compositor& _Compositor;
	EntityManager& _ECS;
	AssetManager& _Assets;

	std::shared_ptr<ScreenResizeEvent> _ScreenResizeEvent;

	std::vector<gpu::RenderPass> _UserInterfaceRP;

	gpu::Semaphore _AcquireNextImageSem;
	gpu::Semaphore _EndOfFrameSem;

	void RenderGBuffer(const Camera& camera, CameraRender& cameraRender, gpu::CommandBuffer& cmdBuf);
	void RenderShadowDepths(CameraRender& camera, gpu::CommandBuffer& cmdBuf);
	void ComputeDirectLighting(CameraRender& camera, gpu::CommandBuffer& cmdBuf);
	void ComputeDirectLighting(CameraRender& camera, gpu::CommandBuffer& cmdBuf, const struct DirectLightingParams& light, bool isFirstLight);
	void ComputeSSGI(const Camera& camera, CameraRender& cameraRender, gpu::CommandBuffer& cmdBuf);
	void ComputeRayTracing(const Camera& camera, CameraRender& cameraRender, gpu::CommandBuffer& cmdBuf);
	void RenderSkybox(CameraRender& camera, gpu::CommandBuffer& cmdBuf);
	void ComputePostProcessing(const gpu::Image& displayImage, CameraRender& camera, gpu::CommandBuffer& cmdBuf);
	void RenderUserInterface(gpu::CommandBuffer& cmdBuf, const gpu::RenderPass& renderPass);

	void CreateUserInterfacePipeline();
};
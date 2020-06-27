#pragma once
#include <DRM.h>
#include "CameraProxy.h"

class Engine;
class Camera;

class SceneRenderer
{
public:
	SceneRenderer(Engine& Engine);

	void Render(CameraProxy& Camera);

private:
	drm::Device& Device;
	drm::ShaderLibrary& ShaderLibrary;
	drm::Surface& Surface;
	EntityManager& ECS;
	AssetManager& Assets;
	Camera& _Camera;

	void ClearSceneColor(CameraProxy& Camera, drm::CommandList& CmdList);
	void RenderGBufferPass(CameraProxy& Camera, drm::CommandList& CmdList);
	void RenderShadowDepths(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeLightingPass(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeIndirectLightingPass(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeDeferredLight(CameraProxy& Camera, drm::CommandList& CmdList, const struct LightData& Light);
	void ComputeRayTracing(CameraProxy& Camera, drm::CommandList& CmdList);
	void RenderSkybox(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputePostProcessing(const drm::Image& DisplayImage, CameraProxy& Camera, drm::CommandList& CmdList);
};
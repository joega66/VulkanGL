#pragma once
#include <DRM.h>
#include "CameraProxy.h"

class SceneRenderer
{
public:
	SceneRenderer(class Engine& Engine);

	void Render(CameraProxy& Camera);

private:
	drm::Device& Device;
	drm::ShaderLibrary& ShaderLibrary;
	drm::Surface& Surface;
	EntityManager& ECS;
	AssetManager& Assets;

	void RenderGBufferPass(CameraProxy& Camera, drm::CommandList& CmdList);
	void RenderShadowDepths(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeLightingPass(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeIndirectLightingPass(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeDeferredLight(CameraProxy& Camera, drm::CommandList& CmdList, const struct LightData& Light);
	void RenderSkybox(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputePostProcessing(const drm::Image& DisplayImage, CameraProxy& Camera, drm::CommandList& CmdList);
};
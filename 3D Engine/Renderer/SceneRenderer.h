#pragma once
#include <DRM.h>
#include "CameraProxy.h"

class SceneRenderer
{
public:
	SceneRenderer(class Engine& Engine);

	void Render(CameraProxy& Camera);

private:
	DRMDevice& Device;
	DRMShaderMap& ShaderMap;
	drm::Surface& Surface;
	EntityManager& ECS;
	AssetManager& Assets;

	void RenderGBufferPass(CameraProxy& Camera, drm::CommandList& CmdList);
	void RenderShadowDepths(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeLightingPass(CameraProxy& Camera, drm::CommandList& CmdList);
	void ComputeDeferredLight(CameraProxy& Camera, drm::CommandList& CmdList, const struct LightParams& Light);
	void RenderSkybox(CameraProxy& Camera, drm::CommandList& CmdList);
	void Present(CameraProxy& Camera, drm::CommandList& CmdList);
};
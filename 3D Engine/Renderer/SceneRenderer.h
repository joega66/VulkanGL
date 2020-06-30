#pragma once
#include <GPU/GPU.h>
#include "CameraProxy.h"

class Engine;
class Camera;

class SceneRenderer
{
public:
	SceneRenderer(Engine& Engine);

	void Render(CameraProxy& Camera);

private:
	gpu::Device& Device;
	gpu::ShaderLibrary& ShaderLibrary;
	gpu::Surface& Surface;
	EntityManager& ECS;
	AssetManager& Assets;
	Camera& _Camera;

	void ClearSceneColor(CameraProxy& Camera, gpu::CommandList& CmdList);
	void RenderGBufferPass(CameraProxy& Camera, gpu::CommandList& CmdList);
	void RenderShadowDepths(CameraProxy& Camera, gpu::CommandList& CmdList);
	void ComputeLightingPass(CameraProxy& Camera, gpu::CommandList& CmdList);
	void ComputeDeferredLight(CameraProxy& Camera, gpu::CommandList& CmdList, const struct LightData& Light);
	void ComputeSSGI(CameraProxy& Camera, gpu::CommandList& CmdList);
	void ComputeRayTracing(CameraProxy& Camera, gpu::CommandList& CmdList);
	void RenderSkybox(CameraProxy& Camera, gpu::CommandList& CmdList);
	void ComputePostProcessing(const gpu::Image& DisplayImage, CameraProxy& Camera, gpu::CommandList& CmdList);
};
#pragma once
#include <GPU/GPU.h>
#include "CameraProxy.h"

class Engine;
class Camera;

class SceneRenderer
{
public:
	SceneRenderer(Engine& engine);

	void Render(CameraProxy& camera);

private:
	gpu::Device& _Device;
	gpu::ShaderLibrary& _ShaderLibrary;
	gpu::Surface& _Surface;
	EntityManager& _ECS;
	AssetManager& _Assets;
	Camera& _Camera;

	gpu::DescriptorSet _PostProcessingSet;

	void RenderGBufferPass(CameraProxy& camera, gpu::CommandList& cmdList);
	void RenderShadowDepths(CameraProxy& camera, gpu::CommandList& cmdList);
	void ComputeLightingPass(CameraProxy& camera, gpu::CommandList& cmdList);
	void ComputeDeferredLight(CameraProxy& camera, gpu::CommandList& cmdList, const struct LightingParams& light, bool isFirstLight);
	void ComputeSSGI(CameraProxy& camera, gpu::CommandList& cmdList);
	void ComputeRayTracing(CameraProxy& camera, gpu::CommandList& cmdList);
	void RenderSkybox(CameraProxy& camera, gpu::CommandList& cmdList);
	void ComputePostProcessing(const gpu::Image& displayImage, CameraProxy& camera, gpu::CommandList& cmdList);
};
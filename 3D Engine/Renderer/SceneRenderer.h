#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/StaticMeshComponent.h>

/** The Scene Renderer renders camera views. */
class SceneRenderer
{
public:
	SceneRenderer(class Engine& Engine);

	void Render(class UserInterface& UserInterface, SceneProxy& Scene);

private:
	DRMDevice& Device;
	DRMShaderMap& ShaderMap;
	EntityManager& ECS;
	AssetManager& Assets;

	void RenderDepthPrepass(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderShadowDepths(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderLightingPass(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderSkybox(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList);
	void Present(drm::CommandList& CmdList);
};
#pragma once
#include <DRM.h>
#include <Renderer/SceneProxy.h>
#include <Components/StaticMeshComponent.h>
#include "Voxels.h"

struct SceneTexturesDescriptors
{
	drm::ImageView Depth;
	drm::ImageView ShadowMask;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static const std::vector<DescriptorBinding> Bindings =
		{ 
			{ 0, 1, SampledImage },
			{ 1, 1, SampledImage },
		};
		return Bindings;
	}
};

/** The Scene Renderer renders camera views and holds persistent render resources. */
class SceneRenderer
{
public:
	SceneRenderer(class Engine& Engine);

	void Render(class UserInterface& UserInterface, SceneProxy& Scene);

	VCTLightingCache VCTLightingCache;

	DescriptorSet<CameraDescriptors> CameraDescriptorSet;
	DescriptorSet<SkyboxDescriptors> SkyboxDescriptorSet;
	DescriptorSet<SceneTexturesDescriptors> SceneTexturesDescriptorSet;

	drm::RenderPass DepthRP;
	drm::RenderPass DepthVisualizationRP;
	drm::RenderPass LightingRP;
	drm::RenderPass ShadowMaskRP;

	drm::Image SceneDepth;
	drm::Image ShadowMask;
	drm::Image SceneColor;

private:
	DRMDevice& Device;
	DRMShaderMap& ShaderMap;
	drm::Surface& Surface;

	void RenderDepthPrepass(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderShadowDepths(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderShadowMask(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderLightingPass(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderSkybox(SceneProxy& Scene, drm::CommandList& CmdList);
	void RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList);
	void Present(drm::CommandList& CmdList);

	void CreateDepthRP();
	void CreateDepthVisualizationRP();
	void CreateLightingRP();
	void CreateShadowMaskRP();

	const StaticMesh* Cube;
};
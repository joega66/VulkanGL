#pragma once
#include "Voxels.h"

struct CameraDescriptors
{
	drm::BufferView CameraUniform;
	drm::BufferView DirectionalLightBuffer;
	drm::BufferView PointLightBuffer;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, UniformBuffer },
			{ 1, 1, StorageBuffer },
			{ 2, 1, StorageBuffer },
		};
		return Bindings;
	}
};

struct SceneTexturesDescriptors
{
	drm::ImageView Depth;
	drm::ImageView RadianceVolume;

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

struct SkyboxDescriptors
{
	drm::ImageView Skybox;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, SampledImage }
		};
		return Bindings;
	}
};

/** Global render data, typically bound to set #0, visible to all passes. */
class GlobalRenderData
{
public:
	GlobalRenderData(class Engine& Engine);

	DRMDevice& Device;
	drm::Surface& Surface;

	VCTLightingCache VCTLightingCache;

	DescriptorSet<CameraDescriptors> CameraDescriptorSet;
	DescriptorSet<SkyboxDescriptors> SkyboxDescriptorSet;
	DescriptorSet<SceneTexturesDescriptors> SceneTexturesDescriptorSet;
	
	drm::RenderPass DepthRP;
	drm::RenderPass DepthVisualizationRP;
	drm::RenderPass LightingRP;

	drm::Image SceneDepth;
	drm::Image SceneColor;

private:
	void CreateDepthRP(DRMDevice& Device);
	void CreateDepthVisualizationRP(DRMDevice& Device);
	void CreateLightingRP(DRMDevice& Device);
};
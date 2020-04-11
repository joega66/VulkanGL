#pragma once
#include "Voxels.h"

struct SkyboxDescriptors
{
	drm::DescriptorImageInfo Skybox;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, EDescriptorType::SampledImage }
		};
		return Bindings;
	}
};

/** Global render data, typically bound to set #0, visible to all passes. */
class GlobalRenderData
{
public:
	GlobalRenderData(class Engine& Engine);

	VCTLightingCache VCTLightingCache;

	DescriptorSet<SkyboxDescriptors> SkyboxDescriptorSet;
};
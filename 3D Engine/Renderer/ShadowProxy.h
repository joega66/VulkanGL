#pragma once
#include <DRM.h>

struct ShadowDescriptors
{
	drm::BufferRef LightViewProjBuffer;
	drm::ImageRef ShadowMap;
	SamplerState ShadowSampler;

	static const std::vector<DescriptorTemplateEntry>& GetEntries()
	{
		static std::vector<DescriptorTemplateEntry> Entries =
		{
			{ 0, 1, UniformBuffer },
			{ 1, 1, SampledImage },
		};
		return Entries;
	}
};

class ShadowProxy
{
public:
	static constexpr EFormat FORMAT = EFormat::D32_SFLOAT;

	ShadowProxy() = default;

	ShadowProxy(DRMDevice& Device, DescriptorTemplate<ShadowDescriptors>& ShadowTemplate, const struct DirectionalLight& DirectionalLight);

	void Update(DRMDevice& Device, const struct DirectionalLight& DirectionalLight);

	/** Accessors */
	inline float GetDepthBiasConstantFactor() const { return DepthBiasConstantFactor; }
	inline float GetDepthBiasSlopeFactor() const { return DepthBiasSlopeFactor; }
	inline drm::RenderPassRef GetRenderPass() const { return RenderPass; };
	inline drm::ImageRef GetShadowMap() const { return ShadowMap; }
	inline drm::DescriptorSetRef GetDescriptorSet() const { return DescriptorSet; }

private:
	drm::RenderPassRef RenderPass;
	float DepthBiasConstantFactor = 0.0f;
	float DepthBiasSlopeFactor = 0.0f;
	drm::BufferRef LightViewProjBuffer;
	drm::ImageRef ShadowMap;
	drm::DescriptorSetRef DescriptorSet;
};
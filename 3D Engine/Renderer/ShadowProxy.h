#pragma once
#include <DRM.h>

struct ShadowDescriptors
{
	const drm::Buffer* LightViewProjBuffer;
	const drm::Image* ShadowMap;
	const drm::Sampler* ShadowSampler;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, UniformBuffer },
			{ 1, 1, SampledImage },
		};
		return Bindings;
	}
};

class ShadowProxy
{
public:
	static constexpr EFormat FORMAT = EFormat::D32_SFLOAT;

	ShadowProxy(DRMDevice& Device, DescriptorSetLayout<ShadowDescriptors>& ShadowLayout, const struct DirectionalLight& DirectionalLight);

	void Update(DRMDevice& Device, const struct DirectionalLight& DirectionalLight);

	/** Accessors */
	inline float GetDepthBiasConstantFactor() const { return DepthBiasConstantFactor; }
	inline float GetDepthBiasSlopeFactor() const { return DepthBiasSlopeFactor; }
	inline const drm::RenderPass& GetRenderPass() const { return RenderPass; };
	inline const drm::Image& GetShadowMap() const { return ShadowMap; }
	inline const drm::DescriptorSet& GetDescriptorSet() const { return DescriptorSet; }

private:
	drm::RenderPass RenderPass;
	float DepthBiasConstantFactor = 0.0f;
	float DepthBiasSlopeFactor = 0.0f;
	drm::Buffer LightViewProjBuffer;
	drm::Image ShadowMap;
	drm::DescriptorSet DescriptorSet;
};
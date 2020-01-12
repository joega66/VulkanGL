#pragma once
#include <DRM.h>

class ShadowProxy
{
public:
	static void InitCallbacks(DRM& Device, class EntityManager& ECS);

	static constexpr EFormat FORMAT = EFormat::D32_SFLOAT;

	ShadowProxy() = default;

	ShadowProxy(DRM& Device, const struct CDirectionalLight& DirectionalLight);

	void Update(DRM& Device, const struct CDirectionalLight& DirectionalLight);

	/** Accessors */
	inline float GetDepthBiasConstantFactor() const { return DepthBiasConstantFactor; }
	inline float GetDepthBiasSlopeFactor() const { return DepthBiasSlopeFactor; }
	inline drm::ImageRef GetShadowMap() const { return ShadowMap; }
	inline drm::DescriptorSetRef GetDescriptorSet() const { return DescriptorSet; }

private:
	float DepthBiasConstantFactor = 0.0f;
	float DepthBiasSlopeFactor = 0.0f;
	drm::BufferRef LightViewProjBuffer;
	drm::ImageRef ShadowMap;
	drm::DescriptorSetRef DescriptorSet;
};
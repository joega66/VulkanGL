#pragma once
#include <Platform/Platform.h>
#include <DRM.h>

class CShadowProxy
{
public:
	static constexpr EFormat FORMAT = EFormat::D32_SFLOAT;

	CShadowProxy() = default;
	CShadowProxy(const struct CDirectionalLight& DirectionalLight);

	void Update(const glm::vec3& Direction);

	/** Accessors */
	inline float GetDepthBiasConstantFactor() const { return DepthBiasConstantFactor; }
	inline float GetDepthBiasSlopeFactor() const { return DepthBiasSlopeFactor; }
	inline drm::ImageRef GetShadowMap() const { return ShadowMap; }
	inline drm::DescriptorSetRef GetDescriptorSet() const { return DescriptorSet; }

private:
	float DepthBiasConstantFactor;
	float DepthBiasSlopeFactor;
	drm::BufferRef LightViewProjBuffer;
	drm::ImageRef ShadowMap;
	drm::DescriptorSetRef DescriptorSet;
};
#pragma once
#include <Platform/Platform.h>
#include <DRM.h>

class ShadowProxy
{
public:
	static void InitCallbacks(class EntityManager& ECS);

	static constexpr EFormat FORMAT = EFormat::D32_SFLOAT;

	ShadowProxy() = default;

	ShadowProxy(const struct CDirectionalLight& DirectionalLight);

	void Update(const struct CDirectionalLight& DirectionalLight);

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
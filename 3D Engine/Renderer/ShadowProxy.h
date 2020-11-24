#pragma once
#include <GPU/GPU.h>
#include <ECS/Component.h>

class ShadowProxy : public Component
{
public:
	float _Width;

	float _ZNear;

	float _ZFar;

	ShadowProxy(gpu::Device& device, const struct DirectionalLight& directionalLight);

	void Update(gpu::Device& device, const struct DirectionalLight& directionalLight, const class Transform& transform);

	inline const gpu::RenderPass& GetRenderPass() const { return _RenderPass; }
	inline gpu::Image& GetShadowMap() { return _ShadowMap; }
	inline const gpu::DescriptorSet& GetDescriptorSet() const { return _DescriptorSet; }
	inline const glm::mat4& GetLightViewProjMatrix() const { return _LightViewProjMatrix; }
	inline const glm::mat4& GetLightViewProjMatrixInv() const { return _LightViewProjMatrixInv; }

	inline float GetDepthBiasConstantFactor() const
	{
		return _DepthBiasConstantFactor;
	}

	inline float GetDepthBiasSlopeFactor() const
	{
		return _DepthBiasSlopeFactor;
	}

private:
	gpu::RenderPass _RenderPass;

	float _DepthBiasConstantFactor = 0.0f;

	float _DepthBiasSlopeFactor = 0.0f;

	glm::mat4 _LightViewProjMatrix;

	glm::mat4 _LightViewProjMatrixInv;

	gpu::Buffer _ShadowUniform;

	gpu::Image _ShadowMap;

	gpu::DescriptorSet _DescriptorSet;
};
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

	void Update(gpu::Device& device, const struct DirectionalLight& directionalLight, const class Transform& transform, uint32 dynamicOffset);

	inline const gpu::RenderPass& GetRenderPass() const { return _RenderPass; }
	inline gpu::Image& GetShadowMap() { return _ShadowMap; }
	inline const glm::mat4& GetLightViewProjMatrix() const { return _LightViewProjMatrix; }
	inline const glm::mat4& GetLightViewProjMatrixInv() const { return _LightViewProjMatrixInv; }
	inline uint32 GetDynamicOffset() const { return _DynamicOffset; }

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

	gpu::Image _ShadowMap;

	uint32 _DynamicOffset;
};
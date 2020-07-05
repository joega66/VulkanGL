#pragma once
#include "MeshDrawCommand.h"
#include <GPU/GPU.h>
#include <ECS/Component.h>

class ShadowProxy : public Component
{
public:
	float _Width;

	float _ZNear;

	float _ZFar;

	ShadowProxy(gpu::Device& device, const struct DirectionalLight& directionalLight);

	void Update(gpu::Device& device, const struct DirectionalLight& directionalLight);

	/** Add a mesh to the light's shadow depth rendering. */
	void AddMesh(gpu::Device& device, gpu::ShaderLibrary& shaderLibrary, const MeshProxy& meshProxy);

	/** Render shadow depths. */
	void Render(gpu::CommandList& cmdList);

	inline const gpu::Image& GetShadowMap() const { return _ShadowMap; }
	inline const gpu::DescriptorSet& GetDescriptorSet() const { return _DescriptorSet; }
	inline const glm::mat4& GetLightViewProjMatrix() const { return _LightViewProjMatrix; }
	inline const glm::mat4& GetLightViewProjMatrixInv() const { return _LightViewProjMatrixInv; }
	inline const glm::vec4& GetL() const { return _L; }
	inline const glm::vec4 GetRadiance() const { return _Radiance; }

private:
	gpu::RenderPass _RenderPass;

	float _DepthBiasConstantFactor = 0.0f;

	float _DepthBiasSlopeFactor = 0.0f;

	glm::mat4 _LightViewProjMatrix;

	glm::mat4 _LightViewProjMatrixInv;

	glm::vec4 _L;

	glm::vec4 _Radiance;

	gpu::Buffer _LightViewProjBuffer;

	gpu::Image _ShadowMap;

	gpu::DescriptorSet _DescriptorSet;

	std::vector<MeshDrawCommand> _MeshDrawCommands;
};
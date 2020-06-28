#pragma once
#include "MeshDrawCommand.h"
#include <GPU/GPU.h>
#include <ECS/Component.h>

struct ShadowDescriptors
{
	gpu::DescriptorBufferInfo LightViewProjBuffer;
	gpu::DescriptorBufferInfo VolumeLightingUniform;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, EDescriptorType::UniformBuffer },
			{ 1, 1, EDescriptorType::UniformBuffer }
		};
		return Bindings;
	}
};

class ShadowProxy : public Component
{
public:
	float Width;

	float ZNear;

	float ZFar;

	ShadowProxy(gpu::Device& Device, DescriptorSetLayout<ShadowDescriptors>& ShadowLayout, const struct DirectionalLight& DirectionalLight);

	void Update(gpu::Device& Device, const struct DirectionalLight& DirectionalLight);

	/** Add a mesh to the light's shadow depth rendering. */
	void AddMesh(gpu::Device& Device, gpu::ShaderLibrary& ShaderLibrary, const MeshProxy& MeshProxy);

	/** Render shadow depths. */
	void Render(gpu::CommandList& CmdList);

	inline const gpu::Image& GetShadowMap() const { return ShadowMap; }
	inline const gpu::DescriptorSet& GetDescriptorSet() const { return DescriptorSet; }
	inline const glm::mat4& GetLightViewProjMatrix() const { return LightViewProjMatrix; }
	inline const glm::mat4& GetLightViewProjMatrixInv() const { return LightViewProjMatrixInv; }
	inline const glm::vec4& GetL() const { return L; }
	inline const glm::vec4 GetRadiance() const { return Radiance; }

private:
	gpu::RenderPass RenderPass;

	float DepthBiasConstantFactor = 0.0f;

	float DepthBiasSlopeFactor = 0.0f;

	glm::mat4 LightViewProjMatrix;

	glm::mat4 LightViewProjMatrixInv;

	glm::vec4 L;

	glm::vec4 Radiance;

	gpu::Buffer LightViewProjBuffer;

	gpu::Image ShadowMap;

	gpu::Buffer VolumeLightingUniform;

	gpu::DescriptorSet DescriptorSet;

	std::vector<MeshDrawCommand> MeshDrawCommands;
};
#pragma once
#include "MeshDrawCommand.h"
#include <DRM.h>
#include <ECS/Component.h>

struct ShadowDescriptors
{
	drm::DescriptorBufferInfo LightViewProjBuffer;
	drm::DescriptorBufferInfo VolumeLightingUniform;

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

	ShadowProxy(drm::Device& Device, DescriptorSetLayout<ShadowDescriptors>& ShadowLayout, const struct DirectionalLight& DirectionalLight);

	void Update(drm::Device& Device, const struct DirectionalLight& DirectionalLight);

	/** Add a mesh to the light's shadow depth rendering. */
	void AddMesh(drm::Device& Device, drm::ShaderLibrary& ShaderLibrary, const MeshProxy& MeshProxy);

	/** Render shadow depths. */
	void Render(drm::CommandList& CmdList);

	inline const drm::Image& GetShadowMap() const { return ShadowMap; }
	inline const drm::DescriptorSet& GetDescriptorSet() const { return DescriptorSet; }
	inline const glm::mat4& GetLightViewProjMatrix() const { return LightViewProjMatrix; }
	inline const glm::mat4& GetLightViewProjMatrixInv() const { return LightViewProjMatrixInv; }
	inline const glm::vec4& GetL() const { return L; }
	inline const glm::vec4 GetRadiance() const { return Radiance; }

private:
	drm::RenderPass RenderPass;

	float DepthBiasConstantFactor = 0.0f;

	float DepthBiasSlopeFactor = 0.0f;

	glm::mat4 LightViewProjMatrix;

	glm::mat4 LightViewProjMatrixInv;

	glm::vec4 L;

	glm::vec4 Radiance;

	drm::Buffer LightViewProjBuffer;

	drm::Image ShadowMap;

	drm::Buffer VolumeLightingUniform;

	drm::DescriptorSet DescriptorSet;

	std::vector<MeshDrawCommand> MeshDrawCommands;
};
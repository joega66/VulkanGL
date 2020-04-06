#pragma once
#include <DRM.h>
#include "MeshDrawCommand.h"

struct ShadowDescriptors
{
	drm::DescriptorBufferInfo LightViewProjBuffer;
	drm::DescriptorImageInfo ShadowMap;
	drm::DescriptorBufferInfo LightInjectionUniform;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, EDescriptorType::UniformBuffer },
			{ 1, 1, EDescriptorType::SampledImage },
			{ 2, 1, EDescriptorType::UniformBuffer }
		};
		return Bindings;
	}
};

class ShadowProxy
{
public:
	ShadowProxy(DRMDevice& Device, DescriptorSetLayout<ShadowDescriptors>& ShadowLayout, const struct DirectionalLight& DirectionalLight);

	void Update(DRMDevice& Device, const struct DirectionalLight& DirectionalLight);

	/** Add a mesh to the light's shadow depth rendering. */
	void AddMesh(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy);

	/** Render shadow depths. */
	void Render(drm::CommandList& CmdList);

	inline const drm::Image& GetShadowMap() const { return ShadowMap; }
	inline const drm::DescriptorSet& GetDescriptorSet() const { return DescriptorSet; }

private:
	drm::RenderPass RenderPass;
	float DepthBiasConstantFactor = 0.0f;
	float DepthBiasSlopeFactor = 0.0f;
	drm::Buffer LightViewProjBuffer;
	drm::Image ShadowMap;
	drm::Buffer LightInjectionUniform;
	drm::DescriptorSet DescriptorSet;
	std::vector<MeshDrawCommand> MeshDrawCommands;
};
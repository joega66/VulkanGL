#pragma once
#include <DRM.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

/** A (very slow) Vulkan object cache. */
class VulkanCache
{
	template<typename DRMObject, typename ...VulkanObjects>
	using SlowCache = std::vector<std::tuple<DRMObject, VulkanObjects...>>;
public:
	VulkanCache(VulkanDevice& Device);

	~VulkanCache();
	
	std::pair<VkRenderPass, VkFramebuffer> GetRenderPass(const RenderPassDesc& RPDesc);

	std::pair<VkPipeline, VkPipelineLayout> GetPipeline(const PipelineStateDesc& PSODesc);

	std::pair<VkPipeline, VkPipelineLayout> GetPipeline(const ComputePipelineDesc& ComputePipelineDesc);

	const drm::Sampler* GetSampler(const SamplerDesc& SamplerDesc);

	std::pair<VkDescriptorSetLayout, VkDescriptorUpdateTemplate> GetDescriptorSetLayout(
		const std::vector<VkDescriptorSetLayoutBinding>& Bindings,
		const std::vector<VkDescriptorUpdateTemplateEntry>& Entries
	);

	void FreeImage(class VulkanImage& Image);

	void DestroyPipelinesWithShader(const drm::Shader* Shader);

	void UpdateDescriptorSetWithTemplate(VkDescriptorSet DescriptorSet, VkDescriptorUpdateTemplate DescriptorUpdateTemplate, const void* Data);

	static const char* GetVulkanErrorString(VkResult Result);

private:
	VulkanDevice& Device;

	SlowCache<RenderPassDesc, VkRenderPass> RenderPassCache;

	SlowCache<std::vector<VkDescriptorSetLayout>, VkPipelineLayout> PipelineLayoutCache;

	SlowCache<PipelineStateDesc, VkPipeline, VkPipelineLayout> GraphicsPipelineCache;

	SlowCache<ComputePipelineDesc, VkPipeline, VkPipelineLayout> ComputePipelineCache;

	PFN_vkUpdateDescriptorSetWithTemplateKHR p_vkUpdateDescriptorSetWithTemplateKHR;

	SlowCache<std::vector<VkDescriptorSetLayoutBinding>, VkDescriptorSetLayout, VkDescriptorUpdateTemplate> DescriptorSetLayoutCache;

	VkPipelineLayout GetPipelineLayout(const std::vector<VkDescriptorSetLayout>& Layouts);

	[[nodiscard]] VkRenderPass CreateRenderPass(const RenderPassDesc& RPDesc);

	[[nodiscard]] VkFramebuffer CreateFramebuffer(VkRenderPass RenderPass, const RenderPassDesc& RPDesc) const;

	[[nodiscard]] VkPipeline CreatePipeline(const PipelineStateDesc& PSODesc, VkPipelineLayout PipelineLayout) const;

	[[nodiscard]] VkPipeline CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc, VkPipelineLayout PipelineLayout) const;

	std::unordered_map<SamplerDesc, drm::Sampler, SamplerDesc::Hash> SamplerCache;
};
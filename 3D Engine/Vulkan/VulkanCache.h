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

	VkPipelineLayout GetPipelineLayout(const std::vector<VkDescriptorSetLayout>& DescriptorSetLayouts);

	VkPipeline GetPipeline(const PipelineStateDesc& PSODesc, VkPipelineLayout PipelineLayout);

	VkSampler GetSampler(const SamplerState& Sampler);

	std::pair<VkDescriptorSetLayout, VkDescriptorUpdateTemplate> 
	GetDescriptorSetLayout(
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

	struct VulkanPipelineHash
	{
		PipelineStateDesc PSODesc;
		VkPipelineLayout PipelineLayout;

		VulkanPipelineHash(const PipelineStateDesc& PSODesc, VkPipelineLayout PipelineLayout);
		bool operator==(const VulkanPipelineHash& Other) const;
		bool HasShader(const drm::Shader* Shader) const;
	};

	SlowCache<VulkanPipelineHash, VkPipeline> PipelineCache;

	PFN_vkUpdateDescriptorSetWithTemplateKHR p_vkUpdateDescriptorSetWithTemplateKHR;

	SlowCache<std::vector<VkDescriptorSetLayoutBinding>, VkDescriptorSetLayout, VkDescriptorUpdateTemplate> DescriptorSetLayoutCache;

	[[nodiscard]] VkRenderPass CreateRenderPass(const RenderPassDesc& RPDesc);

	[[nodiscard]] VkFramebuffer CreateFramebuffer(VkRenderPass RenderPass, const RenderPassDesc& RPDesc) const;

	[[nodiscard]] VkPipeline CreatePipeline(const PipelineStateDesc& PSODesc, VkPipelineLayout PipelineLayout);

	std::unordered_map<SamplerState, VkSampler, SamplerState::Hash> SamplerCache;
};
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

	std::pair<VkPipelineLayout, VkPushConstantRange> GetPipelineLayout(const std::vector<VkDescriptorSetLayout>& Layouts, const PushConstantRange& PushConstantRange);

	drm::Pipeline GetPipeline(const PipelineStateDesc& PSODesc);

	drm::Pipeline GetPipeline(const ComputePipelineDesc& ComputePipelineDesc);

	drm::Sampler GetSampler(const SamplerDesc& SamplerDesc);

	std::pair<VkDescriptorSetLayout, VkDescriptorUpdateTemplate> GetDescriptorSetLayout(
		const std::vector<VkDescriptorSetLayoutBinding>& Bindings,
		const std::vector<VkDescriptorUpdateTemplateEntry>& Entries
	);

	inline void Destroy(VkPipeline Pipeline) { PipelinesToDestroy.push_back(Pipeline); }

	void EndFrame();

	void UpdateDescriptorSetWithTemplate(VkDescriptorSet DescriptorSet, VkDescriptorUpdateTemplate DescriptorUpdateTemplate, const void* Data);

	void RecompilePipelines();

	static const char* GetVulkanErrorString(VkResult Result);

private:
	VulkanDevice& Device;

	std::unordered_map<std::size_t, VkRenderPass> RenderPassCache;

	std::unordered_map<Crc, std::pair<VkDescriptorSetLayout, VkDescriptorUpdateTemplate>> SetLayoutCache;

	std::unordered_map<Crc, VkPipelineLayout> PipelineLayoutCache;

	std::unordered_map<PipelineStateDesc, drm::Pipeline> GraphicsPipelineCache;

	std::unordered_map<Crc, drm::Pipeline> ComputePipelineCache;
	std::unordered_map<Crc, ComputePipelineDesc> CrcToComputeDesc;

	std::unordered_map<Crc, drm::Sampler> SamplerCache;

	PFN_vkUpdateDescriptorSetWithTemplateKHR p_vkUpdateDescriptorSetWithTemplateKHR;

	[[nodiscard]] VkRenderPass CreateRenderPass(const RenderPassDesc& RPDesc);

	[[nodiscard]] VkFramebuffer CreateFramebuffer(VkRenderPass RenderPass, const RenderPassDesc& RPDesc) const;

	[[nodiscard]] VkPipeline CreatePipeline(const PipelineStateDesc& PSODesc, VkPipelineLayout PipelineLayout) const;

	[[nodiscard]] VkPipeline CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc, VkPipelineLayout PipelineLayout) const;

	std::vector<VkPipeline> PipelinesToDestroy;
};
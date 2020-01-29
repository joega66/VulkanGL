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
	
	std::pair<VkRenderPass, VkFramebuffer> GetRenderPass(const RenderPassInitializer& RPInit);

	VkPipelineLayout GetPipelineLayout(const std::vector<VkDescriptorSetLayout>& DescriptorSetLayouts);

	VkPipeline GetPipeline(
		const PipelineStateInitializer& PSOInit, 
		VkPipelineLayout PipelineLayout, 
		VkRenderPass RenderPass, 
		uint32 NumRenderTargets);

	VkSampler GetSampler(const SamplerState& Sampler);

	VkDescriptorSetLayout GetDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& Bindings);

	void FreeImage(class VulkanImage& Image);

	void DestroyPipelinesWithShader(const drm::ShaderRef& Shader);

	static const char* GetVulkanErrorString(VkResult Result);

private:
	VulkanDevice& Device;

	SlowCache<RenderPassInitializer, VkRenderPass, VkFramebuffer> RenderPassCache;

	SlowCache<std::vector<VkDescriptorSetLayout>, VkPipelineLayout> PipelineLayoutCache;

	struct VulkanPipelineHash
	{
		PipelineStateInitializer PSOInit;
		VkPipelineLayout PipelineLayout;
		VkRenderPass RenderPass;
		uint32 NumAttachments;

		VulkanPipelineHash(const PipelineStateInitializer& PSOInit, VkPipelineLayout PipelineLayout, VkRenderPass RenderPass, uint32 NumRenderTargets);
		bool operator==(const VulkanPipelineHash& Other) const;
		bool HasShader(const drm::ShaderRef& Shader) const;
	};

	SlowCache<VulkanPipelineHash, VkPipeline> PipelineCache;

	SlowCache<std::vector<VkDescriptorSetLayoutBinding>, VkDescriptorSetLayout> DescriptorSetLayoutCache;

	[[nodiscard]] std::pair<VkRenderPass, VkFramebuffer> CreateRenderPass(const RenderPassInitializer& RPInit);

	[[nodiscard]] VkPipeline CreatePipeline(
		const PipelineStateInitializer& PSOInit,
		VkPipelineLayout PipelineLayout,
		VkRenderPass RenderPass,
		uint32 NumRenderTargets);

	std::unordered_map<SamplerState, VkSampler, SamplerState::Hash> SamplerCache;
};
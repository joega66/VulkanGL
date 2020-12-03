#pragma once
#include <GPU/GPU.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanCache
{
public:
	VulkanCache(VulkanDevice& Device);

	~VulkanCache();
	
	std::pair<VkRenderPass, VkFramebuffer> GetRenderPass(const RenderPassDesc& RPDesc);

	VkPipelineLayout GetPipelineLayout(
		const std::vector<VkDescriptorSetLayout>& Layouts, 
		const std::vector<VkPushConstantRange>& PushConstantRanges
	);

	gpu::Pipeline GetPipeline(const PipelineStateDesc& PSODesc);

	gpu::Pipeline GetPipeline(const ComputePipelineDesc& ComputePipelineDesc);

	gpu::Sampler GetSampler(const SamplerDesc& SamplerDesc);

	inline void Destroy(VkPipeline Pipeline) { PipelinesToDestroy.push_back(Pipeline); }

	void EndFrame();

	void RecompilePipelines();

private:
	VulkanDevice& Device;

	std::unordered_map<std::size_t, VkRenderPass> RenderPassCache;

	std::unordered_map<Crc, VkPipelineLayout> PipelineLayoutCache;

	std::unordered_map<PipelineStateDesc, gpu::Pipeline> GraphicsPipelineCache;

	std::unordered_map<Crc, gpu::Pipeline> ComputePipelineCache;
	std::unordered_map<Crc, ComputePipelineDesc> CrcToComputeDesc;

	std::unordered_map<Crc, gpu::Sampler> SamplerCache;

	[[nodiscard]] VkRenderPass CreateRenderPass(const RenderPassDesc& RPDesc);

	[[nodiscard]] VkFramebuffer CreateFramebuffer(VkRenderPass RenderPass, const RenderPassDesc& RPDesc) const;

	[[nodiscard]] VkPipeline CreatePipeline(const PipelineStateDesc& PSODesc, VkPipelineLayout PipelineLayout) const;

	[[nodiscard]] VkPipeline CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc, VkPipelineLayout PipelineLayout) const;

	std::vector<VkPipeline> PipelinesToDestroy;
};
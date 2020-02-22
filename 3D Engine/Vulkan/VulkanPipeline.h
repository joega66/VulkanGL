#pragma once
#include <vulkan/vulkan.h>

class VulkanPipeline
{
public:
	VulkanPipeline() = default;
	VulkanPipeline(VkPipeline Pipeline, VkPipelineLayout PipelineLayout);

	inline VkPipeline GetPipeline() const { return Pipeline; }
	inline VkPipelineLayout GetPipelineLayout() const { return PipelineLayout; }

private:
	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
};
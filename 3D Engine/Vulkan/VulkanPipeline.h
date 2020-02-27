#pragma once
#include <vulkan/vulkan.h>

class VulkanPipeline
{
public:
	VulkanPipeline() = default;
	VulkanPipeline(VkPipeline Pipeline, VkPipelineLayout PipelineLayout, VkPipelineBindPoint PipelineBindPoint);

	inline VkPipeline GetPipeline() const { return Pipeline; }
	inline VkPipelineLayout GetPipelineLayout() const { return PipelineLayout; }
	inline VkPipelineBindPoint GetPipelineBindPoint() const { return PipelineBindPoint; }

private:
	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
	VkPipelineBindPoint PipelineBindPoint;
};
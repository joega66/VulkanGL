#pragma once
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanPipeline
{
	friend class VulkanCache;
public:
	VulkanPipeline() = delete;
	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline& operator=(const VulkanPipeline&) = delete;
	VulkanPipeline(VulkanDevice& Device, 
		VkPipeline Pipeline, 
		VkPipelineLayout PipelineLayout, 
		VkPipelineBindPoint PipelineBindPoint);
	~VulkanPipeline();

	inline VkPipeline GetPipeline() const { return Pipeline; }
	inline VkPipelineLayout GetPipelineLayout() const { return PipelineLayout; }
	inline VkPipelineBindPoint GetPipelineBindPoint() const { return PipelineBindPoint; }

private:
	VulkanDevice& Device;
	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
	VkPipelineBindPoint PipelineBindPoint;
};
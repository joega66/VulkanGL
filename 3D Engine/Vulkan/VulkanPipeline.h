#pragma once
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanPipeline
{
	friend class VulkanDevice;
public:
	VulkanPipeline() = delete;
	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline& operator=(const VulkanPipeline&) = delete;
	VulkanPipeline(VulkanDevice& device, 
		VkPipeline pipeline, 
		VkPipelineLayout pipelineLayout, 
		VkPipelineBindPoint pipelineBindPoint);
	~VulkanPipeline();

	inline VkPipeline GetPipeline() const { return _Pipeline; }
	inline VkPipelineLayout GetPipelineLayout() const { return _PipelineLayout; }
	inline VkPipelineBindPoint GetPipelineBindPoint() const { return _PipelineBindPoint; }

private:
	VulkanDevice& _Device;
	VkPipeline _Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout _PipelineLayout = VK_NULL_HANDLE;
	VkPipelineBindPoint _PipelineBindPoint;
};
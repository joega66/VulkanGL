#pragma once
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanScopedCommandBuffer
{
public:
	VulkanScopedCommandBuffer(VulkanDevice& Device, VkQueueFlags QueueFlags);

	~VulkanScopedCommandBuffer();

	operator VkCommandBuffer() { return CommandBuffer; }

private:
	VulkanDevice& Device;
	VkQueue Queue;
	VkCommandPool CommandPool;
	VkCommandBuffer CommandBuffer;
};
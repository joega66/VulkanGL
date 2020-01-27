#pragma once
#include <vulkan/vulkan.h>

class VulkanDRM;

class VulkanScopedCommandBuffer
{
public:
	VulkanScopedCommandBuffer(VulkanDRM& Device, VkQueueFlags QueueFlags);

	~VulkanScopedCommandBuffer();

	operator VkCommandBuffer() { return CommandBuffer; }

private:
	VulkanDRM& Device;
	VkQueue Queue;
	VkCommandPool CommandPool;
	VkCommandBuffer CommandBuffer;
};
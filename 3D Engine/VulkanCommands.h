#pragma once
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanScopedCommandBuffer
{
public:
	VulkanScopedCommandBuffer(VulkanDevice& Device);
	~VulkanScopedCommandBuffer();

	operator VkCommandBuffer() { return CommandBuffer; }

private:
	VulkanDevice& Device;
	VkCommandBuffer CommandBuffer;
};
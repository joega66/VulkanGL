#pragma once
#include "Platform.h"
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanAllocator
{
public:
	VulkanAllocator(VulkanDevice& Device);
	uint32 FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties);

private:
	VulkanDevice& Device;
};
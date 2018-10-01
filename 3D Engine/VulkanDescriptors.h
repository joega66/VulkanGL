#pragma once
#include "Platform.h"
#include <vulkan/vulkan.h>

class VulkanDevice;

struct VulkanWriteDescriptorImage
{
	VkWriteDescriptorSet WriteDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	VkDescriptorImageInfo DescriptorImage = {};
};

class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(VulkanDevice& Device);
	operator VkDescriptorPool() { return DescriptorPool; }

private:
	VkDescriptorPool DescriptorPool;
	VulkanDevice& Device;
};
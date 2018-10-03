#pragma once
#include "Platform.h"
#include <vulkan/vulkan.h>

class VulkanDevice;

struct VulkanWriteDescriptorImage
{
	const VkDescriptorSetLayoutBinding& Binding;
	VkDescriptorImageInfo DescriptorImage = {};

	VulkanWriteDescriptorImage(const VkDescriptorSetLayoutBinding& Binding, const VkDescriptorImageInfo& DescriptorImage)
		: Binding(Binding), DescriptorImage(DescriptorImage)
	{
	}
};

struct VulkanWriteDescriptorBuffer
{
	const VkDescriptorSetLayoutBinding& Binding;
	VkDescriptorBufferInfo DescriptorBuffer = {};

	VulkanWriteDescriptorBuffer(const VkDescriptorSetLayoutBinding& Binding, const VkDescriptorBufferInfo& DescriptorBuffer)
		: Binding(Binding), DescriptorBuffer(DescriptorBuffer)
	{
	}
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
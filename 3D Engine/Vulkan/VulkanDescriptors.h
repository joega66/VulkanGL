#pragma once
#include "Platform/Platform.h"
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

/** Spawns descriptor sets and zerglings */
class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(VulkanDevice& Device);
	[[nodiscard]] VkDescriptorSet Spawn(const VkDescriptorSetLayout& DescriptorSetLayout);
	void Reset();

private:
	uint32 DescriptorSetCount;
	const uint32 MaxDescriptorSetCount;
	VkDescriptorPool DescriptorPool;
	VulkanDevice& Device;
};
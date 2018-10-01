#include "VulkanDescriptors.h"
#include "VulkanDevice.h"
#include <array>

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice& Device)
	: Device(Device)
{
	std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE> PoolSizes;
	uint32 MaxDescriptorSetCount = 4096;

	for (VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; DescriptorType < VK_DESCRIPTOR_TYPE_RANGE_SIZE;)
	{
		VkDescriptorPoolSize& PoolSize = PoolSizes[DescriptorType];
		PoolSize.descriptorCount = MaxDescriptorSetCount;
		PoolSize.type = DescriptorType;
		DescriptorType = VkDescriptorType(DescriptorType + 1);
	}

	VkDescriptorPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.poolSizeCount = PoolSizes.size();
	PoolInfo.maxSets = MaxDescriptorSetCount;

	vulkan(vkCreateDescriptorPool(Device, &PoolInfo, nullptr, &DescriptorPool));
}

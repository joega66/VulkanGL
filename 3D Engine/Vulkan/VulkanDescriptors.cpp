#include "VulkanDescriptors.h"
#include "VulkanDevice.h"

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice& Device)
	: Device(Device)
	, MaxDescriptorSetCount(4096)
	, DescriptorSetCount(0)
{
	std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE> PoolSizes;

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

VkDescriptorSet VulkanDescriptorPool::Spawn(const VkDescriptorSetLayout& DescriptorSetLayout)
{
	check(DescriptorSetCount < MaxDescriptorSetCount, "Descriptor pool is full.");

	VkDescriptorSetAllocateInfo DescriptorSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	DescriptorSetInfo.descriptorPool = DescriptorPool;
	DescriptorSetInfo.descriptorSetCount = 1;
	DescriptorSetInfo.pSetLayouts = &DescriptorSetLayout;

	VkDescriptorSet DescriptorSet;
	vulkan(vkAllocateDescriptorSets(Device, &DescriptorSetInfo, &DescriptorSet));
	DescriptorSetCount++;

	return DescriptorSet;
}

void VulkanDescriptorPool::Reset()
{
	vulkan(vkResetDescriptorPool(Device, DescriptorPool, 0));
	DescriptorSetCount = 0;
}
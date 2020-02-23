#pragma once
#include <DRMResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanDescriptorSet
{
public:
	VulkanDescriptorSet() = default;

	VulkanDescriptorSet(class VulkanDescriptorPool& DescriptorPool, 
		VkDescriptorSetLayout Layout, 
		VkDescriptorSet DescriptorSet
	);
	VulkanDescriptorSet(VulkanDescriptorSet&& Other);
	VulkanDescriptorSet& operator=(VulkanDescriptorSet&& Other);
	VulkanDescriptorSet(const VulkanDescriptorSet&) = delete;
	VulkanDescriptorSet& operator=(const VulkanDescriptorSet&) = delete;
	~VulkanDescriptorSet();

	inline VkDescriptorSet GetVulkanHandle() const { return DescriptorSet; }
	inline VkDescriptorSetLayout GetLayout() const { return Layout; }

private:
	VulkanDescriptorPool* DescriptorPool = nullptr;
	VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
	VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
};

/** Spawns descriptor sets and zerglings. */
class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(VulkanDevice& Device, const VkDescriptorPoolCreateInfo& CreateInfo);

	/** Allocate a descriptor set. */
	[[nodiscard]] VkDescriptorSet Allocate(VulkanDevice& Device, VkDescriptorSetLayout Layout);

	/** Queue a descriptor set to be freed. */
	void Free(VkDescriptorSet DescriptorSet);

	/** Free all the descriptor sets waiting to be freed. */
	void DeferredFree(VulkanDevice& Device);

private:
	VkDescriptorPool DescriptorPool;
	uint32 NumDescriptorSets = 0;
	std::vector<VkDescriptorSet> DescriptorSetsWaitingToBeFreed;
};

class VulkanDescriptorPoolManager
{
public:
	static constexpr uint32 SetsPerPool = 1024;

	VulkanDescriptorPoolManager();

	VulkanDescriptorSet Allocate(VulkanDevice& Device, VkDescriptorSetLayout Layout);

	void DeferredFree(VulkanDevice& Device);

private:
	std::vector<std::unique_ptr<VulkanDescriptorPool>> DescriptorPools;
	std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE> PoolSizes;
	VkDescriptorPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
};

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout() = default;
	VulkanDescriptorSetLayout(VulkanDevice& Device, uint32 NumEntries, const DescriptorBinding* Entries);
	VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& Other);
	VulkanDescriptorSetLayout& operator=(VulkanDescriptorSetLayout&& Other);
	VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
	VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

	VulkanDescriptorSet CreateDescriptorSet();

	void UpdateDescriptorSet(const VulkanDescriptorSet& DescriptorSet, void* Data);

	inline VkDescriptorSetLayout GetNativeHandle() const { return DescriptorSetLayout; }

private:
	VulkanDevice* Device = nullptr;
	VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorUpdateTemplate DescriptorUpdateTemplate = VK_NULL_HANDLE;
	std::vector<VkDescriptorUpdateTemplateEntry> DescriptorUpdateTemplateEntries;
};
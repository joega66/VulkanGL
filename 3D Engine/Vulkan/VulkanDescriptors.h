#pragma once
#include <GPU/GPUResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanDescriptorSet
{
public:
	VulkanDescriptorSet(const VulkanDescriptorSet&) = delete;
	VulkanDescriptorSet& operator=(const VulkanDescriptorSet&) = delete;

	VulkanDescriptorSet() = default;
	VulkanDescriptorSet(class VulkanDescriptorPool& DescriptorPool, VkDescriptorSet DescriptorSet);
	VulkanDescriptorSet(VulkanDescriptorSet&& Other);
	VulkanDescriptorSet& operator=(VulkanDescriptorSet&& Other);
	~VulkanDescriptorSet();

	inline operator VkDescriptorSet() const { return DescriptorSet; }
	inline const VkDescriptorSet& GetHandle() const { return DescriptorSet; }

private:
	VulkanDescriptorPool* DescriptorPool = nullptr;
	VkDescriptorSet DescriptorSet = nullptr;
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
	void EndFrame(VulkanDevice& Device);

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

	void EndFrame(VulkanDevice& Device);

private:
	std::vector<std::unique_ptr<VulkanDescriptorPool>> DescriptorPools;
	std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE> PoolSizes;
	VkDescriptorPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
};

namespace gpu { class Device; }

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout() = default;

	VulkanDescriptorSetLayout(VulkanDevice& Device, std::size_t NumEntries, const DescriptorBinding* Entries);

	VulkanDescriptorSet CreateDescriptorSet(gpu::Device& Device);
	void UpdateDescriptorSet(gpu::Device& Device, const VulkanDescriptorSet& DescriptorSet, void* Data);

	inline operator VkDescriptorSetLayout() const { return DescriptorSetLayout; }
	inline VkDescriptorSetLayout GetHandle() const { return DescriptorSetLayout; }

private:
	VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorUpdateTemplate DescriptorUpdateTemplate = VK_NULL_HANDLE;
};
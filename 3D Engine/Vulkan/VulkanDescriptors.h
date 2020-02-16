#pragma once
#include <DRMResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanDescriptorSet : public drm::DescriptorSet
{
public:
	VulkanDescriptorSet(
		class VulkanDescriptorPool& DescriptorPool,
		VkDescriptorSetLayout Layout,
		VkDescriptorSet DescriptorSet
	);

	virtual ~VulkanDescriptorSet() override;

	inline VkDescriptorSet GetVulkanHandle() const { return DescriptorSet; }
	inline VkDescriptorSetLayout GetLayout() const { return Layout; }

private:
	VulkanDescriptorPool& DescriptorPool;
	const VkDescriptorSetLayout Layout;
	const VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
};

CLASS(VulkanDescriptorSet);

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

	VulkanDescriptorSetRef Allocate(VulkanDevice& Device, VkDescriptorSetLayout Layout);

	void DeferredFree(VulkanDevice& Device);

private:
	std::vector<std::unique_ptr<VulkanDescriptorPool>> DescriptorPools;
	std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE> PoolSizes;
	VkDescriptorPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
};

class VulkanDescriptorTemplate : public drm::DescriptorTemplate
{
public:
	VulkanDescriptorTemplate(VulkanDevice& Device, uint32 NumEntries, const DescriptorTemplateEntry* Entries);
	
	virtual ~VulkanDescriptorTemplate() override;

	virtual drm::DescriptorSetRef CreateDescriptorSet() override;

	virtual void UpdateDescriptorSet(drm::DescriptorSetRef DescriptorSet, void* Data) override;

private:
	VulkanDevice& Device;
	VkDescriptorSetLayout DescriptorSetLayout;
	VkDescriptorUpdateTemplate DescriptorUpdateTemplate;
	std::vector<VkDescriptorUpdateTemplateEntry> DescriptorUpdateTemplateEntries;
	void* Data = nullptr;
};

CLASS(VulkanDescriptorTemplate);
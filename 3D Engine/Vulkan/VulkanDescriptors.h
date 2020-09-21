#pragma once
#include <GPU/GPUResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

namespace gpu
{
	class Device;

	class DescriptorSet
	{
	public:
		DescriptorSet(const DescriptorSet&) = delete;
		DescriptorSet& operator=(const DescriptorSet&) = delete;

		DescriptorSet() = default;
		DescriptorSet(class DescriptorPool& descriptorPool, VkDescriptorSet descriptorSet);
		DescriptorSet(DescriptorSet&& Other);
		DescriptorSet& operator=(DescriptorSet&& Other);
		~DescriptorSet();

		inline operator VkDescriptorSet() const { return _DescriptorSet; }
		inline const VkDescriptorSet& GetHandle() const { return _DescriptorSet; }

	private:
		DescriptorPool* DescriptorPool = nullptr;
		VkDescriptorSet _DescriptorSet = nullptr;
	};

	/** Spawns descriptor sets and zerglings. */
	class DescriptorPool
	{
	public:
		DescriptorPool(VulkanDevice& Device, const VkDescriptorPoolCreateInfo& CreateInfo);

		/** Allocate a descriptor set. */
		[[nodiscard]] VkDescriptorSet Allocate(VulkanDevice& Device, VkDescriptorSetLayout Layout);

		/** Queue a descriptor set to be freed. */
		void Free(VkDescriptorSet DescriptorSet);

		/** Free all the descriptor sets waiting to be freed. */
		void EndFrame(VulkanDevice& Device);

	private:
		VkDescriptorPool _DescriptorPool;
		uint32 NumDescriptorSets = 0;
		std::vector<VkDescriptorSet> DescriptorSetsWaitingToBeFreed;
	};

	class DescriptorPoolManager
	{
	public:
		static constexpr uint32 SetsPerPool = 1024;

		DescriptorPoolManager();

		DescriptorSet Allocate(VulkanDevice& Device, VkDescriptorSetLayout Layout);

		void EndFrame(VulkanDevice& Device);

	private:
		std::vector<std::unique_ptr<DescriptorPool>> DescriptorPools;
		std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE> PoolSizes;
		VkDescriptorPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	};

	class DescriptorSetLayout
	{
	public:
		DescriptorSetLayout() = default;

		DescriptorSetLayout(VulkanDevice& Device, std::size_t NumEntries, const DescriptorBinding* Entries);

		DescriptorSet CreateDescriptorSet(gpu::Device& Device);
		void UpdateDescriptorSet(gpu::Device& Device, const DescriptorSet& DescriptorSet, void* Data);

		inline operator VkDescriptorSetLayout() const { return _DescriptorSetLayout; }
		inline VkDescriptorSetLayout GetHandle() const { return _DescriptorSetLayout; }

	private:
		VkDescriptorSetLayout _DescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorUpdateTemplate DescriptorUpdateTemplate = VK_NULL_HANDLE;
	};
};
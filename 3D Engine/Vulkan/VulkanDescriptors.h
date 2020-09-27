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
		DescriptorSet(DescriptorSet&& other);
		DescriptorSet& operator=(DescriptorSet&& other);
		~DescriptorSet();

		inline operator VkDescriptorSet() const { return _DescriptorSet; }
		inline const VkDescriptorSet& GetHandle() const { return _DescriptorSet; }

	private:
		DescriptorPool* _DescriptorPool = nullptr;
		VkDescriptorSet _DescriptorSet = nullptr;
	};

	/** Spawns descriptor sets and zerglings. */
	class DescriptorPool
	{
	public:
		DescriptorPool(VulkanDevice& device, const VkDescriptorPoolCreateInfo& descriptorPoolInfo);

		/** Allocate a descriptor set. */
		[[nodiscard]] VkDescriptorSet Allocate(VulkanDevice& device, VkDescriptorSetLayout layout);

		/** Queue a descriptor set to be freed. */
		void Free(VkDescriptorSet descriptorSet);

		/** Free all the descriptor sets waiting to be freed. */
		void EndFrame(VulkanDevice& device);

	private:
		VkDescriptorPool _DescriptorPool;
		uint32 _NumDescriptorSets = 0;
		std::vector<VkDescriptorSet> _DescriptorSetsWaitingToBeFreed;
	};

	class DescriptorPoolManager
	{
	public:
		static constexpr uint32 SETS_PER_POOL = 1024;

		DescriptorPoolManager();

		DescriptorSet Allocate(VulkanDevice& device, VkDescriptorSetLayout layout);

		void EndFrame(VulkanDevice& device);

	private:
		std::vector<std::unique_ptr<DescriptorPool>> _DescriptorPools;
		std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE> _PoolSizes;
		VkDescriptorPoolCreateInfo _PoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	};

	class DescriptorSetLayout
	{
	public:
		DescriptorSetLayout() = default;

		DescriptorSetLayout(VulkanDevice& device, std::size_t numBindings, const DescriptorBinding* bindings);

		DescriptorSet CreateDescriptorSet(gpu::Device& device);

		void UpdateDescriptorSet(gpu::Device& device, const DescriptorSet& descriptorSet, const void* data);

		inline operator VkDescriptorSetLayout() const { return _DescriptorSetLayout; }

	private:
		VkDescriptorSetLayout _DescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorUpdateTemplate _DescriptorUpdateTemplate = VK_NULL_HANDLE;
	};
};
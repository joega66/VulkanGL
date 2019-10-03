#pragma once
#include <Platform/Platform.h>
#include <DRMResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

/** Spawns descriptor sets and zerglings */
class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(VulkanDevice& Device);

	[[nodiscard]] VkDescriptorSet Spawn(const VkDescriptorSetLayout& DescriptorSetLayout);

	void Free(VkDescriptorSet DescriptorSet);

	void EndFrame();

private:
	static constexpr uint32 MaxDescriptorSetCount = 4096;

	VulkanDevice& Device;

	uint32 DescriptorSetCount = 0;

	VkDescriptorPool DescriptorPool;

	uint32 PendingFreeCount = 0;

	std::array<VkDescriptorSet, MaxDescriptorSetCount> PendingFreeDescriptorSets;
};

class VulkanAllocator;

class VulkanDescriptorSet : public drm::DescriptorSet
{
public:
	VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;

	VulkanDescriptorSet(VulkanDevice& Device, VulkanDescriptorPool& DescriptorPool, VulkanAllocator& Allocator);
	~VulkanDescriptorSet();

	virtual void Write(drm::ImageRef Image, const SamplerState& Sampler, const ShaderBinding& Binding) override;
	virtual void Write(drm::UniformBufferRef UniformBuffer, const ShaderBinding& Binding) override;
	virtual void Write(drm::StorageBufferRef StorageBuffer, const ShaderBinding& Binding) override;
	virtual void Update() override;

private:
	std::once_flag SpawnDescriptorSetOnceFlag;

	VulkanDevice& Device;
	VulkanDescriptorPool& DescriptorPool;
	VulkanAllocator& Allocator;

	std::vector<VkDescriptorSetLayoutBinding> VulkanBindings;
	std::vector<VkWriteDescriptorSet> PendingWrites;

	void MaybeAddBinding(const ShaderBinding& Binding, VkDescriptorType DescriptorType);
};

CLASS(VulkanDescriptorSet);
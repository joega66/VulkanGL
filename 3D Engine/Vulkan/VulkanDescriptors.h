#pragma once
#include <Platform/Platform.h>
#include <DRMResource.h>
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

class VulkanDescriptorPool;
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
	VulkanDevice& Device;
	VulkanDescriptorPool& DescriptorPool;
	VulkanAllocator& Allocator;

	std::vector<VkDescriptorSetLayoutBinding> VulkanBindings;
	std::vector<VkWriteDescriptorSet> PendingWrites;

	void MaybeAddBinding(const ShaderBinding& Binding, VkDescriptorType DescriptorType);
};

CLASS(VulkanDescriptorSet);
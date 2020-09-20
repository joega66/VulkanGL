#pragma once
#include <GPU/GPUShader.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <optional>

class VulkanDevice;

class VulkanBuffer : public gpu::BufferPrivate
{
public:
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	VulkanBuffer() = default;
	VulkanBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, const VmaAllocationInfo& allocationInfo, VkDeviceSize size, EBufferUsage usage);
	VulkanBuffer(VulkanBuffer&& other);
	VulkanBuffer& operator=(VulkanBuffer&& other);
	~VulkanBuffer();
	
	inline void* GetData() { return _AllocationInfo.pMappedData; }
	inline VkBuffer GetHandle() const { return _Buffer; }

private:
	VmaAllocator _Allocator;
	VmaAllocation _Allocation;
	VmaAllocationInfo _AllocationInfo;
	VkBuffer _Buffer;
};

class VulkanDescriptorBufferInfo
{
public:
	VulkanDescriptorBufferInfo() = default;
	VulkanDescriptorBufferInfo(const VulkanBuffer& Buffer);

private:
	VkDescriptorBufferInfo DescriptorBufferInfo;
};

namespace gpu
{
	template<typename T>
	class UniformBuffer : public VulkanDescriptorBufferInfo
	{
	public:
		UniformBuffer() = default;
		UniformBuffer(const VulkanBuffer& buffer) : VulkanDescriptorBufferInfo(buffer) {}

		static EDescriptorType GetDescriptorType() { return EDescriptorType::UniformBuffer; }
	};

	class StorageBuffer : public VulkanDescriptorBufferInfo
	{
	public:
		StorageBuffer() = default;
		StorageBuffer(const VulkanBuffer& buffer) : VulkanDescriptorBufferInfo(buffer) {}

		static EDescriptorType GetDescriptorType() { return EDescriptorType::StorageBuffer; }
	};
};
#pragma once
#include <GPU/GPUShader.h>
#include "vk_mem_alloc.h"

class VulkanDevice;

namespace gpu
{
	class Buffer
	{
	public:
		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;

		Buffer() = default;
		Buffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, const VmaAllocationInfo& allocationInfo, VkDeviceSize size, EBufferUsage usage);
		Buffer(Buffer&& other);
		Buffer& operator=(Buffer&& other);
		~Buffer();

		inline uint64 GetSize() const { return _Size; }
		inline void* GetData() { return _AllocationInfo.pMappedData; }
		inline operator const VkBuffer&() const { return _Buffer; }

	private:
		VmaAllocator _Allocator;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocationInfo;
		VkBuffer _Buffer;
		uint64 _Size;
	};

	class DescriptorBufferInfo
	{
	public:
		DescriptorBufferInfo() = default;
		DescriptorBufferInfo(const Buffer& buffer);

	private:
		VkDescriptorBufferInfo _DescriptorBufferInfo;
	};

	template<typename T>
	class UniformBuffer : public DescriptorBufferInfo
	{
	public:
		UniformBuffer() = default;
		UniformBuffer(const Buffer& buffer) : DescriptorBufferInfo(buffer) {}

		static VkDescriptorType GetDescriptorType() { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; }
	};

	class StorageBuffer : public DescriptorBufferInfo
	{
	public:
		StorageBuffer() = default;
		StorageBuffer(const Buffer& buffer) : DescriptorBufferInfo(buffer) {}

		static VkDescriptorType GetDescriptorType() { return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; }
	};
};
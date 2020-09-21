#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace gpu
{
	Buffer::Buffer(
		VmaAllocator allocator,
		VkBuffer buffer,
		VmaAllocation allocation,
		const VmaAllocationInfo& allocationInfo,
		VkDeviceSize size,
		EBufferUsage usage)
		: _Allocator(allocator)
		, _Allocation(allocation)
		, _AllocationInfo(allocationInfo)
		, _Buffer(buffer)
		, _Size(size)
	{
	}

	Buffer::Buffer(Buffer&& other)
		: _Allocator(std::exchange(other._Allocator, nullptr))
		, _Allocation(std::exchange(other._Allocation, nullptr))
		, _AllocationInfo(other._AllocationInfo)
		, _Buffer(std::exchange(other._Buffer, nullptr))
		, _Size(other._Size)
	{
	}

	Buffer& Buffer::operator=(Buffer&& other)
	{
		_Allocator = std::exchange(other._Allocator, nullptr);
		_Allocation = std::exchange(other._Allocation, nullptr);
		_AllocationInfo = other._AllocationInfo;
		_Buffer = std::exchange(other._Buffer, nullptr);
		_Size = other._Size;
		return *this;
	}

	Buffer::~Buffer()
	{
		if (_Allocator)
		{
			vmaDestroyBuffer(_Allocator, _Buffer, _Allocation);
		}
	}

	DescriptorBufferInfo::DescriptorBufferInfo(const Buffer& buffer)
	{
		_DescriptorBufferInfo.buffer = buffer.GetHandle();
		_DescriptorBufferInfo.offset = 0;
		_DescriptorBufferInfo.range = buffer.GetSize();
	}
};
#include "VulkanMemory.h"
#include "VulkanDevice.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

VulkanBuffer::VulkanBuffer(
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
	, BufferPrivate(usage, size)
{
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other)
	: _Allocator(std::exchange(other._Allocator, nullptr))
	, _Allocation(std::exchange(other._Allocation, nullptr))
	, _AllocationInfo(other._AllocationInfo)
	, _Buffer(std::exchange(other._Buffer, nullptr))
	, gpu::BufferPrivate(other.GetUsage(), other.GetSize())
{
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other)
{
	_Allocator = std::exchange(other._Allocator, nullptr);
	_Allocation = std::exchange(other._Allocation, nullptr);
	_AllocationInfo = other._AllocationInfo;
	_Buffer = std::exchange(other._Buffer, nullptr);
	_Usage = other._Usage;
	_Size = other._Size;
	return *this;
}

VulkanBuffer::~VulkanBuffer()
{
	if (_Allocator)
	{
		vmaDestroyBuffer(_Allocator, _Buffer, _Allocation);
	}
}

VulkanDescriptorBufferInfo::VulkanDescriptorBufferInfo(const VulkanBuffer& Buffer)
{
	DescriptorBufferInfo.buffer = Buffer.GetHandle();
	DescriptorBufferInfo.offset = 0;
	DescriptorBufferInfo.range = Buffer.GetSize();
}
#pragma once
#include "GLRenderResource.h"
#include <vulkan/vulkan.h>

class VulkanDevice;

struct VulkanBuffer
{
	VkBuffer Buffer;
	VkDeviceMemory Memory;
	VkBufferUsageFlags Usage;
	VkMemoryPropertyFlags Properties;
	VkDeviceSize Size;
	VkDeviceSize Used;

	VulkanBuffer(VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size)
		: Buffer(Buffer), Memory(Memory), Usage(Usage), Properties(Properties), Size(Size), Used(0)
	{
	}

	VkDeviceSize SizeRemaining() const
	{
		return Size - Used;
	}
};

struct SharedVulkanBuffer
{
	VulkanBuffer& Buffer;
	VkDeviceSize Size;
	VkDeviceSize Offset;
	
	SharedVulkanBuffer(VulkanBuffer& Buffer, VkDeviceSize Size)
		: Buffer(Buffer), Size(Size), Offset(Buffer.Used)
	{
		Buffer.Used += Size;
	}

	VkBuffer& GetVulkanHandle() const
	{
		return Buffer.Buffer;
	}
};

class VulkanAllocator
{
public:
	VulkanAllocator(VulkanDevice& Device);
	SharedVulkanBuffer CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags VulkanUsage, EResourceUsageFlags Usage, const void* Data = nullptr);
	uint32 FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties);
	void UploadBufferData(const SharedVulkanBuffer& Buffer, const void* Data);

private:
	VulkanDevice& Device;
	const VkDeviceSize BufferAllocationSize;

	std::map<std::pair<VkBuffer, VkDeviceSize>, std::unique_ptr<VulkanBuffer>> LockedStagingBuffers;
	std::list<std::unique_ptr<VulkanBuffer>> FreeStagingBuffers;
	std::list<VulkanBuffer> Buffers;

	VulkanBuffer CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);
	void* LockBuffer(const SharedVulkanBuffer& Buffer);
	void UnlockBuffer(const SharedVulkanBuffer& Buffer);
};

class VulkanVertexBuffer : public GLVertexBuffer
{
public:
	SharedVulkanBuffer Buffer;

	VulkanVertexBuffer(const SharedVulkanBuffer& Buffer, EImageFormat Format, EResourceUsageFlags Usage)
		: Buffer(Buffer), GLVertexBuffer(Format, Usage)
	{
	}
};

CLASS(VulkanVertexBuffer);

class VulkanIndexBuffer : public GLIndexBuffer
{
public:
	SharedVulkanBuffer Buffer;

	VulkanIndexBuffer(const SharedVulkanBuffer& Buffer, uint32 IndexStride, EImageFormat Format, EResourceUsageFlags Usage)
		: Buffer(Buffer), GLIndexBuffer(IndexStride, Format, Usage)
	{
	}
};

CLASS(VulkanIndexBuffer);

class VulkanUniformBuffer : public GLUniformBuffer
{
public:
	SharedVulkanBuffer Buffer;
	bool bDirty = false;

	VulkanUniformBuffer(const SharedVulkanBuffer& Buffer)
		: Buffer(Buffer)
	{
	}

	virtual uint32 Size() final { return (uint32)Buffer.Size; }

private:
	virtual void MarkDirty() final { bDirty = true; }
};

CLASS(VulkanUniformBuffer);
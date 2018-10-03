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

	enum
	{
		// Allocate in 2MB chunks
		BufferAllocationSize = 2 * (1 << 20),
	};

	std::map<std::pair<VkBuffer, VkDeviceSize>, std::unique_ptr<VulkanBuffer>> LockedStagingBuffers;
	std::list<std::unique_ptr<VulkanBuffer>> FreeStagingBuffers;
	std::vector<VulkanBuffer> Buffers;

	VulkanBuffer CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);
	void* LockBuffer(const SharedVulkanBuffer& Buffer);
	void UnlockBuffer(const SharedVulkanBuffer& Buffer);
};

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
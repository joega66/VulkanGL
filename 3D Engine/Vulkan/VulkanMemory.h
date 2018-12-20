#pragma once
#include "../GLRenderResource.h"
#include "VulkanImage.h"

class VulkanDevice;

struct VulkanBuffer
{
	VkBuffer Buffer;
	VkDeviceMemory Memory;
	VkBufferUsageFlags Usage;
	VkMemoryPropertyFlags Properties;
	VkDeviceSize Size;
	VkDeviceSize Used;

	VulkanBuffer(VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size);
	VkDeviceSize SizeRemaining() const;
	std::shared_ptr<struct SharedVulkanBuffer> Allocate(VkDeviceSize Size);

private:
	friend struct SharedVulkanBuffer;

	struct Slot
	{
		VkDeviceSize Offset;
		VkDeviceSize Size;
	};

	std::list<Slot> FreeList;

	void Free(const struct SharedVulkanBuffer& SharedBuffer);
};

struct SharedVulkanBuffer
{
	// @todo Make VulkanBuffer a shared_ptr
	VulkanBuffer& Shared;
	VkDeviceSize Size;
	VkDeviceSize Offset;
	
	SharedVulkanBuffer(VulkanBuffer& Buffer, VkDeviceSize Size, VkDeviceSize Offset);
	VkBuffer& GetVulkanHandle() const;
	~SharedVulkanBuffer();
};

CLASS(SharedVulkanBuffer);

class VulkanAllocator
{
public:
	VulkanAllocator(VulkanDevice& Device);
	SharedVulkanBufferRef CreateBuffer(
		VkDeviceSize Size, 
		VkBufferUsageFlags VulkanUsage, 
		EResourceUsageFlags Usage, 
		const void* Data = nullptr);
	uint32 FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties);
	void UploadBufferData(const SharedVulkanBuffer& Buffer, const void* Data);
	void* LockBuffer(const SharedVulkanBuffer& Buffer);
	void UnlockBuffer(const SharedVulkanBuffer& Buffer);
	void UploadImageData(const VulkanImageRef Image, const uint8* Pixels);

private:
	VulkanDevice& Device;
	const VkDeviceSize BufferAllocationSize;

	std::map<VkImage, std::unique_ptr<VulkanBuffer>> LockedStagingImages;
	std::map<std::pair<VkBuffer, VkDeviceSize>, std::unique_ptr<VulkanBuffer>> LockedStagingBuffers;
	std::list<std::unique_ptr<VulkanBuffer>> FreeStagingBuffers;
	std::list<VulkanBuffer> Buffers;

	[[nodiscard]] VulkanBuffer CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);
	void* LockBuffer(VkBufferUsageFlags Usage, VkDeviceSize Size, /** Size is the size of the VulkanBuffer, not Shared */
		std::function<void(std::unique_ptr<VulkanBuffer> StagingBuffer)>&& LockStagingBuffer, const SharedVulkanBuffer* Buffer = nullptr);
	void UnlockImage(const VulkanImageRef Image, VkDeviceSize Size);
};

class VulkanVertexBuffer : public GLVertexBuffer
{
public:
	SharedVulkanBufferRef Buffer;

	VulkanVertexBuffer(SharedVulkanBufferRef Buffer, EImageFormat Format, EResourceUsageFlags Usage)
		: Buffer(Buffer), GLVertexBuffer(Format, Usage)
	{
	}
};

CLASS(VulkanVertexBuffer);

class VulkanIndexBuffer : public GLIndexBuffer
{
public:
	SharedVulkanBufferRef Buffer;

	VulkanIndexBuffer(SharedVulkanBufferRef Buffer, uint32 IndexStride, EImageFormat Format, EResourceUsageFlags Usage)
		: Buffer(Buffer), GLIndexBuffer(IndexStride, Format, Usage)
	{
	}
};

CLASS(VulkanIndexBuffer);

class VulkanUniformBuffer : public GLUniformBuffer
{
public:
	SharedVulkanBufferRef Buffer;
	bool bDirty = false;

	VulkanUniformBuffer(SharedVulkanBufferRef Buffer)
		: Buffer(Buffer)
	{
	}

	virtual uint32 GetSize() final { return (uint32)Buffer->Size; }

	~VulkanUniformBuffer()
	{
	}

private:
	virtual void MarkDirty() final { bDirty = true; }
};

CLASS(VulkanUniformBuffer);
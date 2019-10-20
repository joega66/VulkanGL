#pragma once
#include "../DRMResource.h"
#include "VulkanImage.h"

class VulkanDevice;

struct VulkanBuffer
{
	friend struct SharedVulkanBuffer;
public:
	VkBuffer Buffer;

	VkDeviceMemory Memory;

	VkBufferUsageFlags Usage;

	VkMemoryPropertyFlags Properties;

	VkDeviceSize Size;

	VkDeviceSize Used;

	VulkanBuffer(VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size);

	VkDeviceSize SizeRemaining() const;

	static std::shared_ptr<struct SharedVulkanBuffer> Allocate(std::shared_ptr<VulkanBuffer> Buffer, VkDeviceSize Size);

private:
	struct Slot
	{
		VkDeviceSize Offset;
		VkDeviceSize Size;
	};

	std::list<Slot> FreeList;

	void Free(const struct SharedVulkanBuffer& SharedBuffer);
};

CLASS(VulkanBuffer);

struct SharedVulkanBuffer
{
	VulkanBufferRef Shared;
	VkDeviceSize Size;
	VkDeviceSize Offset;
	
	SharedVulkanBuffer(VulkanBufferRef Buffer, VkDeviceSize Size, VkDeviceSize Offset);
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
		EBufferUsage Usage,
		const void* Data = nullptr);

	uint32 FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties) const;

	void UploadBufferData(const SharedVulkanBuffer& Buffer, const void* Data);

	void* LockBuffer(const SharedVulkanBuffer& Buffer);

	void UnlockBuffer(const SharedVulkanBuffer& Buffer);

	void UploadImageData(const VulkanImageRef Image, const uint8* Pixels);

	void UploadCubemapData(const VulkanImageRef Image, const struct CubemapCreateInfo& CubemapCreateInfo);

private:
	VulkanDevice& Device;

	const VkDeviceSize BufferAllocationSize;

	std::map<VkImage, std::unique_ptr<VulkanBuffer>> LockedStagingImages;

	std::map<std::pair<VkBuffer, VkDeviceSize>, std::unique_ptr<VulkanBuffer>> LockedStagingBuffers;

	std::list<std::unique_ptr<VulkanBuffer>> FreeStagingBuffers;

	std::list<VulkanBufferRef> Buffers;

	[[nodiscard]] VulkanBuffer CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);

	void* LockBuffer(VkBufferUsageFlags Usage, VkDeviceSize Size,
		std::function<void(std::unique_ptr<VulkanBuffer> StagingBuffer)>&& LockStagingBuffer, const SharedVulkanBuffer* Buffer = nullptr);

	void UnlockImage(const VulkanImageRef Image, VkDeviceSize Size);
};

class VulkanVertexBuffer : public drm::VertexBuffer
{
public:
	SharedVulkanBufferRef Buffer;

	VulkanVertexBuffer(SharedVulkanBufferRef Buffer, EFormat Format, EBufferUsage Usage)
		: Buffer(Buffer), drm::VertexBuffer(Format, Usage)
	{
	}
};

CLASS(VulkanVertexBuffer);

class VulkanIndexBuffer : public drm::IndexBuffer
{
public:
	SharedVulkanBufferRef Buffer;

	VulkanIndexBuffer(SharedVulkanBufferRef Buffer, uint32 IndexStride, EFormat Format, EBufferUsage Usage)
		: Buffer(Buffer), drm::IndexBuffer(IndexStride, Format, Usage)
	{
	}
};

CLASS(VulkanIndexBuffer);

class VulkanUniformBuffer : public drm::UniformBuffer
{
public:
	SharedVulkanBufferRef Buffer;

	VulkanUniformBuffer(SharedVulkanBufferRef Buffer)
		: Buffer(Buffer)
	{
	}
};

CLASS(VulkanUniformBuffer);

class VulkanStorageBuffer : public drm::StorageBuffer
{
public:
	SharedVulkanBufferRef Buffer;

	VulkanStorageBuffer(SharedVulkanBufferRef Buffer, EBufferUsage Usage)
		: Buffer(Buffer), drm::StorageBuffer(Usage)
	{
	}
};

CLASS(VulkanStorageBuffer);
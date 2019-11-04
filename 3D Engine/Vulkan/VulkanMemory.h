#pragma once
#include "../DRMResource.h"
#include "VulkanImage.h"

class VulkanDevice;

struct VulkanBufferMemory
{
	friend struct SharedVulkanBufferMemory;
public:
	VkBuffer Buffer;

	VkDeviceMemory Memory;

	VkBufferUsageFlags Usage;

	VkMemoryPropertyFlags Properties;

	VkDeviceSize Size;

	VkDeviceSize Used;

	VulkanBufferMemory(VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size);

	VkDeviceSize SizeRemaining() const;

	static std::shared_ptr<struct SharedVulkanBufferMemory> Allocate(std::shared_ptr<VulkanBufferMemory> Buffer, VkDeviceSize Size);

private:
	struct Slot
	{
		VkDeviceSize Offset;
		VkDeviceSize Size;
	};

	std::list<Slot> FreeList;

	void Free(const struct SharedVulkanBufferMemory& SharedBuffer);
};

CLASS(VulkanBufferMemory);

struct SharedVulkanBufferMemory
{
	VulkanBufferMemoryRef Shared;
	VkDeviceSize Size;
	VkDeviceSize Offset;
	
	SharedVulkanBufferMemory(VulkanBufferMemoryRef Buffer, VkDeviceSize Size, VkDeviceSize Offset);
	VkBuffer& GetVulkanHandle() const;
	~SharedVulkanBufferMemory();
};

CLASS(SharedVulkanBufferMemory);

class VulkanAllocator
{
public:
	VulkanAllocator(VulkanDevice& Device);

	SharedVulkanBufferMemoryRef CreateBuffer(
		VkDeviceSize Size, 
		VkBufferUsageFlags VulkanUsage, 
		EBufferUsage Usage,
		const void* Data = nullptr);

	uint32 FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties) const;

	void UploadBufferData(const SharedVulkanBufferMemory& Buffer, const void* Data);

	void* LockBuffer(const SharedVulkanBufferMemory& Buffer);

	void UnlockBuffer(const SharedVulkanBufferMemory& Buffer);

	void UploadImageData(VkCommandBuffer CommandBuffer, const VulkanImageRef Image, const uint8* Pixels);

	void UploadCubemapData(VkCommandBuffer CommandBuffer, const VulkanImageRef Image, const struct CubemapCreateInfo& CubemapCreateInfo);

private:
	VulkanDevice& Device;

	const VkDeviceSize BufferAllocationSize;

	std::map<VkImage, std::unique_ptr<VulkanBufferMemory>> LockedStagingImages;

	std::map<std::pair<VkBuffer, VkDeviceSize>, std::unique_ptr<VulkanBufferMemory>> LockedStagingBuffers;

	std::list<std::unique_ptr<VulkanBufferMemory>> FreeStagingBuffers;

	std::list<VulkanBufferMemoryRef> Buffers;

	[[nodiscard]] VulkanBufferMemory CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);

	void* LockBuffer(VkBufferUsageFlags Usage, VkDeviceSize Size,
		std::function<void(std::unique_ptr<VulkanBufferMemory> StagingBuffer)>&& LockStagingBuffer, const SharedVulkanBufferMemory* Buffer = nullptr);

	void UnlockImage(VkCommandBuffer CommandBuffer, const VulkanImageRef Image, VkDeviceSize Size);
};

class VulkanBuffer : public drm::Buffer
{
public:
	SharedVulkanBufferMemoryRef Buffer;

	VulkanBuffer(SharedVulkanBufferMemoryRef Buffer, EBufferUsage Usage)
		: Buffer(Buffer), drm::Buffer(Usage)
	{
	}
};

CLASS(VulkanBuffer);
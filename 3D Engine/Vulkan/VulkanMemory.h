#pragma once
#include "../DRMResource.h"
#include "VulkanImage.h"

class VulkanDevice;

struct VulkanMemory
{
	friend struct SharedVulkanMemory;
public:
	VkBuffer Buffer;

	VkDeviceMemory Memory;

	VkBufferUsageFlags Usage;

	VkMemoryPropertyFlags Properties;

	VkDeviceSize Size;

	VkDeviceSize Used;

	VulkanMemory(VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size);

	VkDeviceSize SizeRemaining() const;

	static std::shared_ptr<struct SharedVulkanMemory> Allocate(std::shared_ptr<VulkanMemory> Buffer, VkDeviceSize Size);

private:
	struct Slot
	{
		VkDeviceSize Offset;
		VkDeviceSize Size;
	};

	std::list<Slot> FreeList;

	void Free(const struct SharedVulkanMemory& SharedBuffer);
};

CLASS(VulkanMemory);

struct SharedVulkanMemory
{
	VulkanMemoryRef Shared;
	VkDeviceSize Size;
	VkDeviceSize Offset;
	
	SharedVulkanMemory(VulkanMemoryRef Buffer, VkDeviceSize Size, VkDeviceSize Offset);
	VkBuffer& GetVulkanHandle() const;
	~SharedVulkanMemory();
};

CLASS(SharedVulkanMemory);

class VulkanAllocator
{
public:
	VulkanAllocator(VulkanDevice& Device);

	SharedVulkanMemoryRef Allocate(
		VkDeviceSize Size, 
		VkBufferUsageFlags VulkanUsage, 
		EBufferUsage Usage,
		const void* Data = nullptr);

	uint32 FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties) const;

	void UploadBufferData(const SharedVulkanMemory& Buffer, const void* Data);

	void* LockBuffer(const SharedVulkanMemory& Buffer);

	void UnlockBuffer(const SharedVulkanMemory& Buffer);

	void UploadImageData(VkCommandBuffer CommandBuffer, const VulkanImageRef Image, const uint8* Pixels);

	void UploadCubemapData(VkCommandBuffer CommandBuffer, const VulkanImageRef Image, const struct CubemapCreateInfo& CubemapCreateInfo);

private:
	VulkanDevice& Device;

	const VkDeviceSize BufferAllocationSize;

	std::map<VkImage, std::unique_ptr<VulkanMemory>> LockedStagingImages;

	std::map<std::pair<VkBuffer, VkDeviceSize>, std::unique_ptr<VulkanMemory>> LockedStagingBuffers;

	std::list<std::unique_ptr<VulkanMemory>> FreeStagingBuffers;

	std::list<VulkanMemoryRef> Buffers;

	[[nodiscard]] VulkanMemory CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);

	void* LockBuffer(VkBufferUsageFlags Usage, VkDeviceSize Size,
		std::function<void(std::unique_ptr<VulkanMemory> StagingBuffer)>&& LockStagingBuffer, const SharedVulkanMemory* Buffer = nullptr);

	void UnlockImage(VkCommandBuffer CommandBuffer, const VulkanImageRef Image, VkDeviceSize Size);
};

class VulkanBuffer : public drm::Buffer
{
public:
	SharedVulkanMemoryRef Memory;

	VulkanBuffer(SharedVulkanMemoryRef Memory, EBufferUsage Usage)
		: Memory(Memory), drm::Buffer(Usage)
	{
	}
};

CLASS(VulkanBuffer);
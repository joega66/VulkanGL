#pragma once
#include "../DRMResource.h"
#include "VulkanImage.h"

class VulkanDevice;

struct VulkanMemory
{
	friend class VulkanBuffer;
public:
	VulkanMemory(VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size);

	static std::shared_ptr<class VulkanBuffer> Allocate(std::shared_ptr<VulkanMemory> Memory, VkDeviceSize Size, EBufferUsage Usage);

	inline VkBuffer GetVulkanHandle() const { return Buffer; }
	inline VkDeviceMemory GetMemoryHandle() const { return Memory; }
	inline VkBufferUsageFlags GetVulkanUsage() const { return Usage; }
	inline VkMemoryPropertyFlags GetProperties() const { return Properties; }
	inline VkDeviceSize GetSize() const { return Size; }
	inline VkDeviceSize GetSizeRemaining() const { return Size - Used; };

private:
	VkBuffer Buffer;

	VkDeviceMemory Memory;

	VkBufferUsageFlags Usage;

	VkMemoryPropertyFlags Properties;

	VkDeviceSize Size;

	VkDeviceSize Used;

	struct Slot
	{
		VkDeviceSize Offset;
		VkDeviceSize Size;
	};

	std::list<Slot> FreeList;

	void Free(const class VulkanBuffer& Buffer);
};

CLASS(VulkanMemory);

class VulkanAllocator
{
public:
	VulkanAllocator(VulkanDevice& Device);

	std::shared_ptr<class VulkanBuffer> Allocate(
		VkDeviceSize Size, 
		VkBufferUsageFlags VulkanUsage, 
		EBufferUsage Usage,
		const void* Data = nullptr);

	uint32 FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties) const;

	void UploadBufferData(const VulkanBuffer& Buffer, const void* Data);

	void* LockBuffer(const VulkanBuffer& Buffer);

	void UnlockBuffer(const VulkanBuffer& Buffer);

private:
	VulkanDevice& Device;

	const VkDeviceSize BufferAllocationSize;

	std::map<std::pair<VkBuffer, VkDeviceSize>, std::unique_ptr<VulkanMemory>> LockedStagingBuffers;

	std::list<std::unique_ptr<VulkanMemory>> FreeStagingBuffers;

	std::list<VulkanMemoryRef> MemoryBuffers;

	[[nodiscard]] VulkanMemory CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);
};

class VulkanBuffer : public drm::Buffer
{
public:
	VulkanBuffer(VulkanMemoryRef Memory, VkDeviceSize Size, VkDeviceSize Offset, EBufferUsage Usage)
		: Memory(Memory), Size(Size), Offset(Offset), drm::Buffer(Usage)
	{
	}

	~VulkanBuffer();

	inline VkBuffer GetVulkanHandle() const { return Memory->Buffer; }
	inline VkDeviceMemory GetMemoryHandle() const { return Memory->Memory; }
	inline VkDeviceSize GetSize() const { return Size; }
	inline VkDeviceSize GetOffset() const { return Offset; }
	inline VkMemoryPropertyFlags GetProperties() const { return Memory->Properties; }
	
private:
	VulkanMemoryRef Memory;
	VkDeviceSize Size;
	VkDeviceSize Offset;
};

CLASS(VulkanBuffer);
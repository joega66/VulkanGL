#pragma once
#include <DRMResource.h>
#include <vulkan/vulkan.h>
#include <optional>

class VulkanDevice;

class VulkanMemory
{
	friend class VulkanBuffer;
public:
	VulkanMemory(VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size);

	static std::optional<class VulkanBuffer> Allocate(
		std::unique_ptr<VulkanMemory>& Memory,
		VkDeviceSize Size,
		VkDeviceSize AlignedSize,
		EBufferUsage Usage
	);

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

class VulkanBuffer : public drm::BufferPrivate
{
public:
	VulkanBuffer() = default;
	VulkanBuffer(VulkanMemory& Memory, VkDeviceSize Size, VkDeviceSize AlignedSize, VkDeviceSize Offset, EBufferUsage Usage)
		: Memory(&Memory), AlignedSize(AlignedSize), Offset(Offset), drm::BufferPrivate(Usage, static_cast<uint32>(Size)) {}
	VulkanBuffer(VulkanBuffer&& Other);
	VulkanBuffer& operator=(VulkanBuffer&& Other);
	~VulkanBuffer();
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	inline VkBuffer GetVulkanHandle() const { return Memory->Buffer; }
	inline VkDeviceMemory GetMemoryHandle() const { return Memory->Memory; }
	inline VkDeviceSize GetAlignedSize() const { return AlignedSize; }
	inline VkDeviceSize GetOffset() const { return Offset; }
	inline VkMemoryPropertyFlags GetProperties() const { return Memory->Properties; }

private:
	VulkanMemory* Memory;		// The backing memory.
	VkDeviceSize AlignedSize;	// The aligned size of the buffer.
	VkDeviceSize Offset;		// Offset into the memory buffer.
};

class VulkanBufferView
{
public:
	VulkanBufferView() = default;
	VulkanBufferView(const VulkanBuffer& Buffer);

private:
	VkDescriptorBufferInfo DescriptorBufferInfo;
};

class VulkanAllocator
{
public:
	VulkanAllocator(VulkanDevice& Device);

	VulkanBuffer Allocate(
		VkDeviceSize Size,
		VkBufferUsageFlags VulkanUsage,
		EBufferUsage Usage,
		const void* Data = nullptr);

	uint32 FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties) const;

	void UploadBufferData(const class VulkanBuffer& Buffer, const void* Data);

	void* LockBuffer(const class VulkanBuffer& Buffer);

	void UnlockBuffer(const class VulkanBuffer& Buffer);

private:
	VulkanDevice& Device;
	const VkDeviceSize BufferAllocationSize;
	std::vector<std::unique_ptr<VulkanMemory>> MemoryBuffers;

	[[nodiscard]] VulkanMemory AllocateMemory(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);
};

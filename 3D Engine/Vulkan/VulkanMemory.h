#pragma once
#include <GPU/GPUShader.h>
#include <vulkan/vulkan.h>
#include <optional>

class VulkanDevice;

class VulkanMemory
{
	friend class VulkanBuffer;
public:
	VulkanMemory(VulkanDevice& Device, VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size);

	~VulkanMemory();

	/** Suballocate a VulkanBuffer region. */
	std::optional<class VulkanBuffer> Suballocate(VkDeviceSize Size, VkDeviceSize AlignedSize, EBufferUsage Usage);

	/** Upload a chunk of data to the VulkanBuffer region. */
	void UploadBufferData(VulkanBuffer& Buffer, const void* Data);

	inline VkBuffer GetVulkanHandle() const { return Buffer; }
	inline VkDeviceMemory GetMemoryHandle() const { return Memory; }
	inline VkBufferUsageFlags GetVulkanUsage() const { return Usage; }
	inline VkMemoryPropertyFlags GetProperties() const { return Properties; }
	inline VkDeviceSize GetSize() const { return Size; }
	inline VkDeviceSize GetSizeRemaining() const { return Size - Used; };
	inline void* GetData() { return Data; }

private:
	VulkanDevice& Device;
	VkBuffer Buffer;
	VkDeviceMemory Memory;
	VkBufferUsageFlags Usage;
	VkMemoryPropertyFlags Properties;
	VkDeviceSize Size;
	VkDeviceSize Used;
	void* Data = nullptr;

	struct Slot
	{
		VkDeviceSize Offset;
		VkDeviceSize Size;
	};

	std::list<Slot> FreeList;

	void Free(const class VulkanBuffer& Buffer);
};

class VulkanBuffer : public gpu::BufferPrivate
{
public:
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	VulkanBuffer() = default;
	VulkanBuffer(VulkanMemory& Memory, VkDeviceSize Size, VkDeviceSize AlignedSize, VkDeviceSize Offset, EBufferUsage Usage)
		: Memory(&Memory), AlignedSize(AlignedSize), Offset(Offset), gpu::BufferPrivate(Usage, static_cast<uint32>(Size)) {}
	VulkanBuffer(VulkanBuffer&& Other);
	VulkanBuffer& operator=(VulkanBuffer&& Other);
	~VulkanBuffer();
	
	inline void* GetData() { return static_cast<uint8*>(Memory->GetData()) + GetOffset(); }
	inline VkBuffer GetHandle() const { return Memory->Buffer; }
	inline VkDeviceSize GetAlignedSize() const { return AlignedSize; }
	inline VkDeviceSize GetOffset() const { return Offset; }
	inline VkMemoryPropertyFlags GetProperties() const { return Memory->Properties; }

private:
	VulkanMemory* Memory;		// The backing memory.
	VkDeviceSize AlignedSize;	// The aligned size of the buffer.
	VkDeviceSize Offset;		// Offset into the memory buffer.
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

	[[nodiscard]] VkDeviceMemory AllocateMemory(uint32 RequiredMemoryTypeBits, VkMemoryPropertyFlags RequiredProperties, VkDeviceSize RequiredSize);

private:
	VulkanDevice& Device;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	const VkDeviceSize BufferAllocationSize;
	std::vector<std::unique_ptr<VulkanMemory>> MemoryBuffers;

	[[nodiscard]] VulkanMemory AllocateMemory(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);
};

class VulkanDescriptorBufferInfo
{
public:
	VulkanDescriptorBufferInfo() = default;
	VulkanDescriptorBufferInfo(const VulkanBuffer& Buffer);

private:
	VkDescriptorBufferInfo DescriptorBufferInfo;
};

namespace gpu
{
	template<typename T>
	class UniformBuffer : public VulkanDescriptorBufferInfo
	{
	public:
		UniformBuffer() = default;
		UniformBuffer(const VulkanBuffer& buffer) : VulkanDescriptorBufferInfo(buffer) {}

		static EDescriptorType GetDescriptorType() { return EDescriptorType::UniformBuffer; }
	};

	class StorageBuffer : public VulkanDescriptorBufferInfo
	{
	public:
		StorageBuffer() = default;
		StorageBuffer(const VulkanBuffer& buffer) : VulkanDescriptorBufferInfo(buffer) {}

		static EDescriptorType GetDescriptorType() { return EDescriptorType::StorageBuffer; }
	};
};
#include "VulkanMemory.h"
#include "VulkanDevice.h"

inline static VkDeviceSize Align(VkDeviceSize Size, VkDeviceSize Alignment)
{
	return (Size + Alignment - 1) & ~(Alignment - 1);
}

VulkanAllocator::VulkanAllocator(VulkanDevice& Device)
	: Device(Device)
	, BufferAllocationSize(20 * (1 << 20))
{
}

VulkanBuffer VulkanAllocator::Allocate(VkDeviceSize Size, VkBufferUsageFlags VulkanUsage, EBufferUsage Usage, const void* Data)
{
	VkMemoryPropertyFlags Properties = Any(Usage & (EBufferUsage::HostVisible | EBufferUsage::Transfer)) ?
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	check((!Data) || (Data && Properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "Buffer needs to be host visible!");

	const VkDeviceSize AlignedSize = [&] ()
	{
		if (VulkanUsage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		{
			return Align(Size, Device.GetProperties().limits.minUniformBufferOffsetAlignment);
		}
		else if (VulkanUsage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		{
			return Align(Size, Device.GetProperties().limits.minStorageBufferOffsetAlignment);
		}
		else
		{
			return Size;
		}
	}();

	for (std::unique_ptr<VulkanMemory>& Memory : MemoryBuffers)
	{
		// Find a buffer with the same properties and usage
		if (Memory->GetVulkanUsage() == VulkanUsage && Memory->GetProperties() == Properties)
		{
			std::optional<VulkanBuffer> Buffer = VulkanMemory::Allocate(Memory, Size, AlignedSize, Usage);

			if (Buffer)
			{
				if (Data)
				{
					UploadBufferData(*Buffer, Data);
				}
				return std::move(Buffer.value());
			}
		}
	}

	// Buffer not found - Create a new one
	MemoryBuffers.emplace_back(std::make_unique<VulkanMemory>(AllocateMemory(Size, VulkanUsage, Properties)));

	std::optional<VulkanBuffer> Buffer = VulkanMemory::Allocate(MemoryBuffers.back(), Size, AlignedSize, Usage);

	if (Data)
	{
		UploadBufferData(Buffer.value(), Data);
	}

	return std::move(Buffer.value());
}

uint32 VulkanAllocator::FindMemoryType(uint32 MemoryTypeBitsRequirement, VkMemoryPropertyFlags RequiredProperties) const
{
	VkPhysicalDeviceMemoryProperties MemProperties;
	vkGetPhysicalDeviceMemoryProperties(Device.GetPhysicalDevice(), &MemProperties);

	const uint32 MemoryCount = MemProperties.memoryTypeCount;
	for (uint32 MemoryIndex = 0; MemoryIndex < MemoryCount; ++MemoryIndex)
	{
		const uint32 MemoryTypeBits = (1 << MemoryIndex);
		const bool IsRequiredMemoryType = MemoryTypeBitsRequirement & MemoryTypeBits;
		const VkMemoryPropertyFlags Properties = MemProperties.memoryTypes[MemoryIndex].propertyFlags;
		const bool HasRequiredProperties = (Properties & RequiredProperties) == RequiredProperties;

		if (IsRequiredMemoryType && HasRequiredProperties)
		{
			return MemoryIndex;
		}
	}

	fail("Suitable memory not found.");
}

void VulkanAllocator::UploadBufferData(const VulkanBuffer& Buffer, const void* Data)
{
	void* MemMapped = LockBuffer(Buffer);
	Platform::Memcpy(MemMapped, Data, (size_t)Buffer.GetSize());
	UnlockBuffer(Buffer);
}

VulkanMemory VulkanAllocator::AllocateMemory(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties)
{
	VkBufferCreateInfo BufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	BufferInfo.size = BufferAllocationSize > Size ? BufferAllocationSize : Size;
	BufferInfo.usage = Usage;

	VkBuffer Buffer;
	vulkan(vkCreateBuffer(Device, &BufferInfo, nullptr, &Buffer));

	VkMemoryRequirements MemRequirements;
	vkGetBufferMemoryRequirements(Device, Buffer, &MemRequirements);

	VkMemoryAllocateInfo MemoryInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	MemoryInfo.allocationSize = MemRequirements.size;
	MemoryInfo.memoryTypeIndex = FindMemoryType(MemRequirements.memoryTypeBits, Properties);

	VkDeviceMemory Memory;
	vulkan(vkAllocateMemory(Device, &MemoryInfo, nullptr, &Memory));
	vkBindBufferMemory(Device, Buffer, Memory, 0);

	return VulkanMemory(Buffer, Memory, Usage, Properties, BufferInfo.size);
}

void* VulkanAllocator::LockBuffer(const VulkanBuffer& Buffer)
{
	check(Buffer.GetProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "Buffer must be host-visible...");
	void* MemMapped = nullptr;
	vulkan(vkMapMemory(Device, Buffer.GetMemoryHandle(), Buffer.GetOffset(), Buffer.GetSize(), 0, &MemMapped));
	return MemMapped;
}

void VulkanAllocator::UnlockBuffer(const VulkanBuffer& Buffer)
{
	check(Buffer.GetProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "Buffer must be host-visible...");
	vkUnmapMemory(Device, Buffer.GetMemoryHandle());
}

VulkanMemory::VulkanMemory(VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size)
	: Buffer(Buffer), Memory(Memory), Usage(Usage), Properties(Properties), Size(Size), Used(0)
{
}

std::optional<VulkanBuffer> VulkanMemory::Allocate(std::unique_ptr<VulkanMemory>& Memory, VkDeviceSize Size, VkDeviceSize AlignedSize, EBufferUsage Usage)
{
	for (auto Iter = Memory->FreeList.begin(); Iter != Memory->FreeList.end(); Iter++)
	{
		if (Iter->Size >= AlignedSize)
		{
			VkDeviceSize Offset = Iter->Offset;

			if (auto Diff = Iter->Size - AlignedSize; Diff > 0)
			{
				Iter->Offset += AlignedSize;
				Iter->Size = Diff;
			}
			else
			{
				Memory->FreeList.erase(Iter);
			}

			return VulkanBuffer(*Memory, Size, AlignedSize, Offset, Usage);
		}
	}

	if (Memory->GetSizeRemaining() >= AlignedSize)
	{
		VulkanBuffer Buffer = VulkanBuffer(*Memory, Size, AlignedSize, Memory->Used, Usage);
		Memory->Used += AlignedSize;
		return Buffer;
	}

	return std::nullopt;
}

void VulkanMemory::Free(const VulkanBuffer& Buffer)
{
	if (Buffer.GetOffset() + Buffer.GetAlignedSize() == Used)
	{
		Used -= Buffer.GetAlignedSize();
		return;
	}

	Slot New = { Buffer.GetOffset(), Buffer.GetAlignedSize() };

	for (auto Iter = FreeList.begin(); Iter != FreeList.end();)
	{
		if (Iter->Offset + Size == New.Offset ||
			Iter->Offset == New.Offset + New.Size)
		{
			New = { std::min(New.Offset, Iter->Offset), Iter->Size + New.Size };
			Iter = FreeList.erase(Iter);
		}
		else
		{
			Iter++;
		}
	}

	FreeList.push_back(New);
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& Other)
	: Memory(std::exchange(Other.Memory, nullptr))
	, AlignedSize(Other.AlignedSize)
	, Offset(Other.Offset)
	, drm::BufferPrivate(Other.Usage, Other.GetSize())
{
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& Other)
{
	Memory = std::exchange(Other.Memory, nullptr);
	AlignedSize = Other.AlignedSize;
	Offset = Other.Offset;
	Usage = Other.Usage;
	Size = Other.Size;
	return *this;
}

VulkanBuffer::~VulkanBuffer()
{
	if (Memory)
	{
		Memory->Free(*this);
	}
}

VulkanBufferView::VulkanBufferView(const VulkanBuffer& Buffer)
{
	DescriptorBufferInfo.buffer = Buffer.GetVulkanHandle();
	DescriptorBufferInfo.offset = Buffer.GetOffset();
	DescriptorBufferInfo.range = Buffer.GetSize();
}
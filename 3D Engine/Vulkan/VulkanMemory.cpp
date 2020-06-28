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
	vkGetPhysicalDeviceMemoryProperties(Device.GetPhysicalDevice(), &MemoryProperties);
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
		else if (VulkanUsage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
		{
			return Align(Size, 4); // VK_INDEX_TYPE_UINT32
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
			std::optional<VulkanBuffer> Buffer = Memory->Suballocate(Size, AlignedSize, Usage);

			if (Buffer)
			{
				if (Data)
				{
					Memory->UploadBufferData(*Buffer, Data);
				}
				return std::move(Buffer.value());
			}
		}
	}

	// Buffer not found - Create a new one
	std::unique_ptr<VulkanMemory>& Memory = MemoryBuffers.emplace_back(std::make_unique<VulkanMemory>(AllocateMemory(Size, VulkanUsage, Properties)));

	std::optional<VulkanBuffer> Buffer = Memory->Suballocate(Size, AlignedSize, Usage);

	if (Data)
	{
		Memory->UploadBufferData(Buffer.value(), Data);
	}

	return std::move(Buffer.value());
}

VkDeviceMemory VulkanAllocator::AllocateMemory(uint32 RequiredMemoryTypeBits, VkMemoryPropertyFlags RequiredProperties, VkDeviceSize RequiredSize)
{
	for (uint32 MemoryIndex = 0; MemoryIndex < MemoryProperties.memoryTypeCount; ++MemoryIndex)
	{
		const uint32 MemoryTypeBits = (1 << MemoryIndex);
		const bool IsRequiredMemoryType = RequiredMemoryTypeBits & MemoryTypeBits;
		const VkMemoryType& MemoryType = MemoryProperties.memoryTypes[MemoryIndex];
		const bool HasRequiredProperties = (MemoryType.propertyFlags & RequiredProperties) == RequiredProperties;

		if (IsRequiredMemoryType && HasRequiredProperties)
		{
			VkMemoryAllocateInfo MemoryInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
			MemoryInfo.allocationSize = RequiredSize;
			MemoryInfo.memoryTypeIndex = MemoryIndex;

			VkDeviceMemory Memory;
			const VkResult Result = vkAllocateMemory(Device, &MemoryInfo, nullptr, &Memory);

			if (Result == VK_ERROR_OUT_OF_DEVICE_MEMORY || Result == VK_ERROR_OUT_OF_HOST_MEMORY)
			{
				continue;
			}

			vulkan(Result);

			return Memory;
		}
	}

	fail("Suitable memory not found.");
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

	const VkDeviceMemory Memory = AllocateMemory(MemRequirements.memoryTypeBits, Properties, MemRequirements.size);
	vkBindBufferMemory(Device, Buffer, Memory, 0);

	return VulkanMemory(Device, Buffer, Memory, Usage, Properties, BufferInfo.size);
}

VulkanMemory::VulkanMemory(VulkanDevice& Device, VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size)
	: Device(Device), Buffer(Buffer), Memory(Memory), Usage(Usage), Properties(Properties), Size(Size), Used(0)
{
	if (Properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		vkMapMemory(Device, Memory, 0, Size, 0, &Data);
	}
}

VulkanMemory::~VulkanMemory()
{
	if (Properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		vkUnmapMemory(Device, Memory);
	}
}

std::optional<VulkanBuffer> VulkanMemory::Suballocate(VkDeviceSize Size, VkDeviceSize AlignedSize, EBufferUsage Usage)
{
	for (auto Iter = FreeList.begin(); Iter != FreeList.end(); Iter++)
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
				FreeList.erase(Iter);
			}

			return VulkanBuffer(*this, Size, AlignedSize, Offset, Usage);
		}
	}

	if (GetSizeRemaining() >= AlignedSize)
	{
		VulkanBuffer Buffer = VulkanBuffer(*this, Size, AlignedSize, Used, Usage);
		Used += AlignedSize;
		return Buffer;
	}

	return std::nullopt;
}

void VulkanMemory::UploadBufferData(VulkanBuffer& Buffer, const void* InData)
{
	Platform::Memcpy(static_cast<uint8*>(Data) + Buffer.GetOffset(), InData, static_cast<size_t>(Buffer.GetSize()));
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
	, gpu::BufferPrivate(Other.GetUsage(), Other.GetSize())
{
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& Other)
{
	Memory = std::exchange(Other.Memory, nullptr);
	AlignedSize = Other.AlignedSize;
	Offset = Other.Offset;
	_Usage = Other._Usage;
	_Size = Other._Size;
	return *this;
}

VulkanBuffer::~VulkanBuffer()
{
	if (Memory)
	{
		Memory->Free(*this);
	}
}

VulkanDescriptorBufferInfo::VulkanDescriptorBufferInfo(const VulkanBuffer& Buffer)
{
	DescriptorBufferInfo.buffer = Buffer.GetHandle();
	DescriptorBufferInfo.offset = Buffer.GetOffset();
	DescriptorBufferInfo.range = Buffer.GetSize();
}
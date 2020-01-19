#include "VulkanMemory.h"
#include "VulkanDevice.h"
#include "VulkanCommands.h"
#include "VulkanQueues.h"
#include <DRM.h>

VulkanAllocator::VulkanAllocator(VulkanDevice& Device, VulkanQueues& Queues)
	: Device(Device)
	, Queues(Queues)
	, BufferAllocationSize(2 * (1 << 20)) // Allocate in 2MB chunks
{
}

VulkanBufferRef VulkanAllocator::Allocate(VkDeviceSize Size, VkBufferUsageFlags VulkanUsage, EBufferUsage Usage, const void* Data)
{
	VkMemoryPropertyFlags Properties = Any(Usage & (EBufferUsage::KeepCPUAccessible | EBufferUsage::Transfer)) ?
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	for (VulkanMemoryRef& Memory : MemoryBuffers)
	{
		// Find a buffer with the same properties and usage
		if (Memory->GetVulkanUsage() == VulkanUsage && Memory->GetProperties() == Properties)
		{
			VulkanBufferRef VulkanBuffer = VulkanMemory::Allocate(Memory, Size, Usage);

			if (VulkanBuffer)
			{
				if (Data)
				{
					UploadBufferData(*VulkanBuffer, Data);
				}
				return VulkanBuffer;
			}
		}
	}

	// Buffer not found - Create a new one
	MemoryBuffers.emplace_back(MakeRef<VulkanMemory>(CreateBuffer(Size, VulkanUsage, Properties)));

	VulkanBufferRef VulkanBuffer = VulkanMemory::Allocate(MemoryBuffers.back(), Size, Usage);

	if (Data)
	{
		UploadBufferData(*VulkanBuffer, Data);
	}

	return VulkanBuffer;
}

uint32 VulkanAllocator::FindMemoryType(uint32 MemoryTypeBitsRequirement, VkMemoryPropertyFlags RequiredProperties) const
{
	VkPhysicalDeviceMemoryProperties MemProperties;
	vkGetPhysicalDeviceMemoryProperties(Device.PhysicalDevice, &MemProperties);

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

VulkanMemory VulkanAllocator::CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties)
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
	if (Buffer.GetProperties() & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	{
		auto StagingBufferIter = std::find_if(FreeStagingBuffers.begin(), FreeStagingBuffers.end(),
			[&] (const std::unique_ptr<VulkanMemory>& StagingBuffer) { return StagingBuffer->GetSize() >= Buffer.GetSize(); });

		std::unique_ptr<VulkanMemory> StagingBuffer;

		if (StagingBufferIter == FreeStagingBuffers.end())
		{
			// No staging buffer of suitable size found - Make a new one
			StagingBuffer = std::make_unique<VulkanMemory>(
				CreateBuffer(BufferAllocationSize > Buffer.GetSize() ? BufferAllocationSize : Buffer.GetSize(),
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			);
		}
		else
		{
			StagingBuffer = std::move(FreeStagingBuffers.back());
			FreeStagingBuffers.pop_back();
		}

		void* MemMapped = nullptr;
		vulkan(vkMapMemory(Device, StagingBuffer->GetMemoryHandle(), 0, Buffer.GetSize(), 0, &MemMapped));

		LockedStagingBuffers[std::make_pair(Buffer.GetVulkanHandle(), Buffer.GetOffset())] = std::move(StagingBuffer);

		return MemMapped;
	}
	else
	{
		void* MemMapped = nullptr;
		vulkan(vkMapMemory(Device, Buffer.GetMemoryHandle(), Buffer.GetOffset(), Buffer.GetSize(), 0, &MemMapped));
		return MemMapped;
	}
}

void VulkanAllocator::UnlockBuffer(const VulkanBuffer& Buffer)
{
	if (Buffer.GetProperties() & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	{
		auto Key = std::make_pair(Buffer.GetVulkanHandle(), Buffer.GetOffset());

		std::unique_ptr<VulkanMemory> StagingBuffer = std::move(LockedStagingBuffers[Key]);
		LockedStagingBuffers.erase(Key);

		vkUnmapMemory(Device, StagingBuffer->GetMemoryHandle());

		VulkanScopedCommandBuffer CommandBuffer(Device, Queues, VK_QUEUE_TRANSFER_BIT);

		VkBufferCopy Copy = {};
		Copy.size = Buffer.GetSize();
		Copy.dstOffset = Buffer.GetOffset();
		Copy.srcOffset = 0;

		vkCmdCopyBuffer(CommandBuffer, StagingBuffer->GetVulkanHandle(), Buffer.GetVulkanHandle(), 1, &Copy);

		FreeStagingBuffers.push_back(std::move(StagingBuffer));
	}
	else
	{
		vkUnmapMemory(Device, Buffer.GetMemoryHandle());
	}
}

VulkanMemory::VulkanMemory(VkBuffer Buffer, VkDeviceMemory Memory, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkDeviceSize Size)
	: Buffer(Buffer), Memory(Memory), Usage(Usage), Properties(Properties), Size(Size), Used(0)
{
}

std::shared_ptr<VulkanBuffer> VulkanMemory::Allocate(std::shared_ptr<VulkanMemory> Memory, VkDeviceSize Size, EBufferUsage Usage)
{
	for (auto Iter = Memory->FreeList.begin(); Iter != Memory->FreeList.end(); Iter++)
	{
		if (Iter->Size >= Size)
		{
			VkDeviceSize Offset = Iter->Offset;

			if (auto Diff = Iter->Size - Size; Diff > 0)
			{
				Iter->Offset += Size;
				Iter->Size = Diff;
			}
			else
			{
				Memory->FreeList.erase(Iter);
			}

			return MakeRef<VulkanBuffer>(Memory, Size, Offset, Usage);
		}
	}

	if (Memory->GetSizeRemaining() >= Size)
	{
		VulkanBufferRef VulkanBuffer = MakeRef<class VulkanBuffer>(Memory, Size, Memory->Used, Usage);
		Memory->Used += Size;
		return VulkanBuffer;
	}

	return nullptr;
}

void VulkanMemory::Free(const VulkanBuffer& Buffer)
{
	if (Buffer.GetOffset() + Buffer.GetSize() == Used)
	{
		Used -= Buffer.GetSize();
		return;
	}

	Slot New = { Buffer.GetOffset(), Buffer.GetSize() };

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

VulkanBuffer::~VulkanBuffer()
{
	Memory->Free(*this);
}
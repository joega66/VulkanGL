#include "VulkanMemory.h"
#include "VulkanDevice.h"
#include "VulkanCommands.h"

VulkanAllocator::VulkanAllocator(VulkanDevice& Device) 
	: Device(Device)
{
}

SharedVulkanBuffer VulkanAllocator::CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags VulkanUsage, EResourceUsageFlags Usage, const void* Data)
{
	check(Size > 0, "Buffer size must be > 0.");

	VulkanUsage |= Usage & RU_UnorderedAccess ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
	VulkanUsage |= Usage & RU_IndirectBuffer ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;
	VulkanUsage |= !(Usage & RU_KeepCPUAccessible) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0; 

	VkMemoryPropertyFlags Properties = Usage & RU_KeepCPUAccessible ? 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	for (uint32 i = 0; i < Buffers.size(); i++)
	{
		// Find a buffer with the same properties and usage

		VulkanBuffer& Buffer = Buffers[i];
		if (Buffer.Usage == VulkanUsage && Buffer.Properties == Properties)
		{
			if (Buffer.SizeRemaining() >= Size)
			{
				SharedVulkanBuffer SharedVulkanBuffer(Buffer, Size);

				if (Data)
				{
					UploadBufferData(SharedVulkanBuffer, Data);
				}

				return SharedVulkanBuffer;
			}
		}
	}

	// Buffer not found - Create a new one
	VulkanBuffer VulkanBuffer = CreateBuffer(Size, VulkanUsage, Properties);
	Buffers.push_back(VulkanBuffer);

	SharedVulkanBuffer SharedVulkanBuffer(Buffers.back(), Size);

	if (Data)
	{
		UploadBufferData(SharedVulkanBuffer, Data);
	}

	return SharedVulkanBuffer;
}

uint32 VulkanAllocator::FindMemoryType(uint32 MemoryTypeBitsRequirement, VkMemoryPropertyFlags RequiredProperties)
{
	VkPhysicalDeviceMemoryProperties MemProperties;
	vkGetPhysicalDeviceMemoryProperties(Device, &MemProperties);

	const uint32_t MemoryCount = MemProperties.memoryTypeCount;
	for (uint32_t MemoryIndex = 0; MemoryIndex < MemoryCount; ++MemoryIndex)
	{
		const uint32_t MemoryTypeBits = (1 << MemoryIndex);
		const bool IsRequiredMemoryType = MemoryTypeBitsRequirement & MemoryTypeBits;

		const VkMemoryPropertyFlags Properties =
			MemProperties.memoryTypes[MemoryIndex].propertyFlags;
		const bool HasRequiredProperties =
			(Properties & RequiredProperties) == RequiredProperties;

		if (IsRequiredMemoryType && HasRequiredProperties)
		{
			return MemoryIndex;
		}
	}

	fail("Suitable memory not found.");
}

void VulkanAllocator::UploadBufferData(const SharedVulkanBuffer& Buffer, const void* Data)
{
	void* MemMapped = LockBuffer(Buffer);
	GPlatform->Memcpy(MemMapped, Data, (size_t)Buffer.Size);
	UnlockBuffer(Buffer);
}

VulkanBuffer VulkanAllocator::CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties)
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

	return VulkanBuffer(Buffer, Memory, Usage, Properties, BufferInfo.size);
}

void* VulkanAllocator::LockBuffer(const SharedVulkanBuffer& Buffer)
{
	if (Buffer.Buffer.Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
	{
		auto StagingBufferIter = std::find_if(FreeStagingBuffers.begin(), FreeStagingBuffers.end(),
			[&] (const std::unique_ptr<VulkanBuffer>& StagingBuffer) { return StagingBuffer->Size >= Buffer.Size; });

		std::unique_ptr<VulkanBuffer> StagingBuffer;

		if (StagingBufferIter == FreeStagingBuffers.end())
		{
			// No staging buffer of suitable size found - Make a new one
			VulkanBuffer NewStagingBuffer = CreateBuffer(
				BufferAllocationSize >= Buffer.Buffer.Size ? BufferAllocationSize : Buffer.Buffer.Size,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			StagingBuffer = std::make_unique<VulkanBuffer>(NewStagingBuffer);
		}
		else
		{
			StagingBuffer = std::move(FreeStagingBuffers.back());
			FreeStagingBuffers.pop_back();
		}

		check(StagingBuffer->Properties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
			"Don't map staging buffers without these flags.");

		void* MemMapped = nullptr;
		vkMapMemory(Device, StagingBuffer->Memory, 0, Buffer.Size, 0, &MemMapped);

		LockedStagingBuffers[std::make_pair(Buffer.Buffer.Buffer, Buffer.Offset)] = std::move(StagingBuffer);

		return MemMapped;
	}
	else
	{
		check(Buffer.Buffer.Properties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
			"CPU-mappable buffer must have these memory properties.");
		void* MemMapped = nullptr;
		vkMapMemory(Device, Buffer.Buffer.Memory, Buffer.Offset, Buffer.Size, 0, &MemMapped);
		return MemMapped;
	}
}

void VulkanAllocator::UnlockBuffer(const SharedVulkanBuffer& Buffer)
{
	if (Buffer.Buffer.Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
	{
		auto Key = std::make_pair(Buffer.Buffer.Buffer, Buffer.Offset);

		std::unique_ptr<VulkanBuffer> StagingBuffer = std::move(LockedStagingBuffers[Key]);
		LockedStagingBuffers.erase(Key);

		check(StagingBuffer, "Buffer was never locked for writing.");

		vkUnmapMemory(Device, StagingBuffer->Memory);

		VulkanScopedCommandBuffer CommandBuffer(Device);

		VkBufferCopy Copy = {};
		Copy.size = Buffer.Size;
		Copy.dstOffset = Buffer.Offset;
		Copy.srcOffset = 0;

		vkCmdCopyBuffer(CommandBuffer, StagingBuffer->Buffer, Buffer.Buffer.Buffer, 1, &Copy);

		FreeStagingBuffers.push_back(std::move(StagingBuffer));
	}
	else
	{
		vkUnmapMemory(Device, Buffer.Buffer.Memory);
	}
}
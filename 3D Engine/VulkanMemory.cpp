#include "VulkanMemory.h"
#include "VulkanDevice.h"
#include "VulkanCommands.h"

VulkanAllocator::VulkanAllocator(VulkanDevice& Device) 
	: Device(Device)
	, BufferAllocationSize(2 * (1 << 20)) // Allocate in 2MB chunks
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

	for (auto& Buffer : Buffers)
	{
		// Find a buffer with the same properties and usage
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

void* VulkanAllocator::LockBuffer(VkBufferUsageFlags Usage, VkDeviceSize Size, std::function<void(std::unique_ptr<VulkanBuffer> StagingBuffer)>&& LockStagingBuffer, const SharedVulkanBuffer* Buffer)
{
	if (Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
	{
		auto StagingBufferIter = std::find_if(FreeStagingBuffers.begin(), FreeStagingBuffers.end(),
			[&] (const std::unique_ptr<VulkanBuffer>& StagingBuffer) { return StagingBuffer->Size >= Size; });

		std::unique_ptr<VulkanBuffer> StagingBuffer;

		if (StagingBufferIter == FreeStagingBuffers.end())
		{
			// No staging buffer of suitable size found - Make a new one
			VulkanBuffer NewStagingBuffer = CreateBuffer(
				BufferAllocationSize >= Size ? BufferAllocationSize : Size,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			StagingBuffer = std::make_unique<VulkanBuffer>(NewStagingBuffer);
		}
		else
		{
			StagingBuffer = std::move(FreeStagingBuffers.back());
			FreeStagingBuffers.pop_back();
		}

		void* MemMapped = nullptr;
		vkMapMemory(Device, StagingBuffer->Memory, 0, Size, 0, &MemMapped);

		LockStagingBuffer(std::move(StagingBuffer));

		return MemMapped;
	}
	else
	{
		check(Buffer && Buffer->Buffer.Properties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
			"CPU-mappable buffer must have these memory properties.");
		void* MemMapped = nullptr;
		vkMapMemory(Device, Buffer->Buffer.Memory, Buffer->Offset, Buffer->Size, 0, &MemMapped);
		return MemMapped;
	}
}

void* VulkanAllocator::LockBuffer(const SharedVulkanBuffer& Buffer)
{
	VulkanBuffer& Backing = Buffer.Buffer;
	return LockBuffer(Backing.Usage, Backing.Size,
		[&] (auto StagingBuffer) 
	{ 
		LockedStagingBuffers[std::make_pair(Buffer.GetVulkanHandle(), Buffer.Offset)] = std::move(StagingBuffer); 
	}, &Buffer
	);
}

void VulkanAllocator::UnlockBuffer(const SharedVulkanBuffer& Buffer)
{
	if (Buffer.Buffer.Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
	{
		auto Key = std::make_pair(Buffer.GetVulkanHandle(), Buffer.Offset);

		std::unique_ptr<VulkanBuffer> StagingBuffer = std::move(LockedStagingBuffers[Key]);
		LockedStagingBuffers.erase(Key);

		vkUnmapMemory(Device, StagingBuffer->Memory);

		VulkanScopedCommandBuffer CommandBuffer(Device);

		VkBufferCopy Copy = {};
		Copy.size = Buffer.Size;
		Copy.dstOffset = Buffer.Offset;
		Copy.srcOffset = 0;

		vkCmdCopyBuffer(CommandBuffer, StagingBuffer->Buffer, Buffer.GetVulkanHandle(), 1, &Copy);

		FreeStagingBuffers.push_back(std::move(StagingBuffer));
	}
	else
	{
		vkUnmapMemory(Device, Buffer.Buffer.Memory);
	}
}

void VulkanAllocator::UploadImageData(const VulkanImageRef Image, const uint8* Pixels)
{
	VkDeviceSize Size = Image->Width * Image->Height;

	switch (Image->Format)
	{
	case IF_R8G8B8A8_UNORM:
		Size *= 4;
		break;
	default:
		signal_unimplemented();
	}

	void* MemMapped = LockBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, Size,
		[&] (auto StagingBuffer)
	{
		LockedStagingImages[Image->Image] = std::move(StagingBuffer);
	});
	GPlatform->Memcpy(MemMapped, Pixels, (size_t)Size);
	UnlockImage(Image);
}

void VulkanAllocator::UnlockImage(const VulkanImageRef Image)
{
	VulkanScopedCommandBuffer CommandBuffer(Device);

	VkBufferImageCopy Region = {};
	Region.bufferOffset = 0;
	Region.bufferRowLength = 0;
	Region.bufferImageHeight = 0;
	Region.imageSubresource.aspectMask = Image->GetVulkanAspect();
	Region.imageSubresource.mipLevel = 0;
	Region.imageSubresource.baseArrayLayer = 0;
	Region.imageSubresource.layerCount = 1;
	Region.imageOffset = { 0, 0, 0 };
	Region.imageExtent = {
		Image->Width,
		Image->Height,
		1
	};

	std::unique_ptr<VulkanBuffer> StagingBuffer = std::move(LockedStagingImages[Image->Image]);
	LockedStagingImages.erase(Image->Image);

	vkCmdCopyBufferToImage(CommandBuffer, StagingBuffer->Buffer, Image->Image, Image->Layout, 1, &Region);

	FreeStagingBuffers.push_back(std::move(StagingBuffer));
}
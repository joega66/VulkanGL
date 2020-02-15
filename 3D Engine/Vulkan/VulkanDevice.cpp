#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanSurface.h"

void VulkanDevice::EndFrame()
{
	DescriptorPoolManager.DeferredFree(*this);
}

void VulkanDevice::SubmitCommands(drm::CommandListRef CmdList)
{
	const VulkanCommandListRef& VulkanCmdList = ResourceCast(CmdList);

	vulkan(vkEndCommandBuffer(VulkanCmdList->CommandBuffer));

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &VulkanCmdList->CommandBuffer;

	vulkan(vkQueueSubmit(VulkanCmdList->Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

	vulkan(vkQueueWaitIdle(VulkanCmdList->Queue));
}

drm::CommandListRef VulkanDevice::CreateCommandList(EQueue Queue)
{
	// @todo Transfer, AsyncCompute Queues
	const VkQueueFlagBits QueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
	return MakeRef<VulkanCommandList>(*this, QueueFlags);
}

drm::DescriptorTemplateRef VulkanDevice::CreateDescriptorTemplate(uint32 NumEntries, const DescriptorTemplateEntry* Entries)
{
	return MakeRef<VulkanDescriptorTemplate>(*this, NumEntries, Entries);
}

drm::BufferRef VulkanDevice::CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data)
{
	VkBufferUsageFlags VulkanUsage = 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Indirect) ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Vertex) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Storage) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Index) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Uniform) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;
	VulkanUsage |= !Any(Usage & EBufferUsage::KeepCPUAccessible) && !Any(Usage & EBufferUsage::Transfer) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Transfer) ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0;

	return Allocator.Allocate(Size, VulkanUsage, Usage, Data);
}

drm::ImageRef VulkanDevice::CreateImage(
	uint32 Width, 
	uint32 Height, 
	uint32 Depth, 
	EFormat Format, 
	EImageUsage UsageFlags, 
	EImageLayout InitialLayout
)
{
	if (drm::Image::IsDepth(Format))
	{
		Format = VulkanImage::GetEngineFormat(VulkanImage::FindSupportedDepthFormat(*this, Format));
	}

	VkImageCreateInfo Info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	Info.imageType = Depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	Info.extent.width = Width;
	Info.extent.height = Height;
	Info.extent.depth = Depth;
	Info.mipLevels = 1;
	Info.arrayLayers = Any(UsageFlags & EImageUsage::Cubemap) ? 6 : 1;
	Info.format = VulkanImage::GetVulkanFormat(Format);
	Info.tiling = VK_IMAGE_TILING_OPTIMAL;
	Info.initialLayout = VulkanImage::GetVulkanLayout(InitialLayout);
	Info.usage = [&] ()
	{
		VkFlags Usage = 0;

		if (Any(UsageFlags & EImageUsage::Attachment))
		{
			Usage |= drm::Image::IsDepth(Format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		Usage |= Any(UsageFlags & EImageUsage::TransferSrc) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
		Usage |= Any(UsageFlags & EImageUsage::TransferDst) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		Usage |= Any(UsageFlags & EImageUsage::Sampled) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
		// Add transfer dst bit to storage images so they can be cleared via ClearColorImage.
		Usage |= Any(UsageFlags & EImageUsage::Storage) ? VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;

		return Usage;
	}();
	Info.flags = Any(UsageFlags & EImageUsage::Cubemap) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	Info.samples = VK_SAMPLE_COUNT_1_BIT;
	Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkImage Image;
	vulkan(vkCreateImage(Device, &Info, nullptr, &Image));

	VkMemoryRequirements MemRequirements = {};
	vkGetImageMemoryRequirements(Device, Image, &MemRequirements);

	VkMemoryAllocateInfo MemInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	MemInfo.allocationSize = MemRequirements.size;
	MemInfo.memoryTypeIndex = Allocator.FindMemoryType(MemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkDeviceMemory Memory;
	vulkan(vkAllocateMemory(Device, &MemInfo, nullptr, &Memory));
	vulkan(vkBindImageMemory(Device, Image, Memory, 0));

	VulkanImageRef VulkanImage = MakeRef<class VulkanImage>(*this
		, Image
		, Memory
		, Format
		, Width
		, Height
		, Depth
		, UsageFlags);

	return VulkanImage;
}

void* VulkanDevice::LockBuffer(drm::BufferRef Buffer)
{
	VulkanBufferRef VulkanBuffer = ResourceCast(Buffer);
	return Allocator.LockBuffer(*VulkanBuffer);
}

void VulkanDevice::UnlockBuffer(drm::BufferRef Buffer)
{
	VulkanBufferRef VulkanBuffer = ResourceCast(Buffer);
	Allocator.UnlockBuffer(*VulkanBuffer);
}

drm::RenderPass VulkanDevice::CreateRenderPass(const RenderPassDesc& RPDesc)
{
	const auto[RenderPass, Framebuffer] = VulkanCache.GetRenderPass(RPDesc);

	const VkRect2D RenderArea = 
	{
		RPDesc.RenderArea.Offset.x,
		RPDesc.RenderArea.Offset.y,
		RPDesc.RenderArea.Extent.x,
		RPDesc.RenderArea.Extent.y
	};

	// Get the clear values from the AttachmentViews.
	std::vector<VkClearValue> ClearValues;

	if (RPDesc.DepthAttachment.Image)
	{
		ClearValues.resize(RPDesc.NumAttachments + 1);
	}
	else
	{
		ClearValues.resize(RPDesc.NumAttachments);
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < RPDesc.NumAttachments; ColorTargetIndex++)
	{
		const auto& ClearValue = std::get<ClearColorValue>(RPDesc.ColorAttachments[ColorTargetIndex].ClearValue);
		memcpy(ClearValues[ColorTargetIndex].color.float32, ClearValue.Float32, sizeof(ClearValue.Float32));
		memcpy(ClearValues[ColorTargetIndex].color.int32, ClearValue.Int32, sizeof(ClearValue.Int32));
		memcpy(ClearValues[ColorTargetIndex].color.uint32, ClearValue.Uint32, sizeof(ClearValue.Uint32));
	}

	if (RPDesc.DepthAttachment.Image)
	{
		VulkanImageRef Image = ResourceCast(RPDesc.DepthAttachment.Image);

		ClearValues[RPDesc.NumAttachments].depthStencil = { 0, 0 };

		if (Image->IsDepth())
		{
			ClearValues[RPDesc.NumAttachments].depthStencil.depth = std::get<ClearDepthStencilValue>(RPDesc.DepthAttachment.ClearValue).DepthClear;
		}

		if (Image->IsStencil())
		{
			ClearValues[RPDesc.NumAttachments].depthStencil.stencil = std::get<ClearDepthStencilValue>(RPDesc.DepthAttachment.ClearValue).StencilClear;
		}
	}

	return VulkanRenderPass(*this, RenderPass, Framebuffer, RenderArea, ClearValues, RPDesc.NumAttachments);
}

void VulkanDevice::CreateLogicalDevice()
{
	const std::unordered_set<int32> UniqueQueueFamilies = Queues.GetUniqueFamilies();
	const float QueuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;

	for (int32 QueueFamily : UniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo QueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		QueueCreateInfo.queueFamilyIndex = QueueFamily;
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(QueueCreateInfo);
	}

	VkPhysicalDeviceFeatures DeviceFeatures = {};
	DeviceFeatures.samplerAnisotropy = VK_TRUE;
	DeviceFeatures.geometryShader = VK_TRUE;
	DeviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
	DeviceFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;

	VkDeviceCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	CreateInfo.queueCreateInfoCount = static_cast<uint32>(QueueCreateInfos.size());
	CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	CreateInfo.pEnabledFeatures = &DeviceFeatures;
	CreateInfo.enabledExtensionCount = static_cast<uint32>(DeviceExtensions.size());
	CreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

	if (Platform::GetBool("Engine.ini", "Renderer", "UseValidationLayers", false))
	{
		CreateInfo.enabledLayerCount = static_cast<uint32>(ValidationLayers.size());
		CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
	}
	else
	{
		CreateInfo.enabledLayerCount = 0;
	}
	
	vulkan(vkCreateDevice(PhysicalDevice, &CreateInfo, nullptr, &Device));

	// Create queues and command pools.
	Queues.Create(Device);
}
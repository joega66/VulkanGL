#include "VulkanDRM.h"
#include "VulkanRenderPass.h"
#include <Engine/Screen.h>

VulkanDRM::VulkanDRM(Platform& Platform, Screen& Screen)
	: Device(Platform, Platform::GetBool("Engine.ini", "Renderer", "UseValidationLayers", false))
	, Allocator(Device)
	, DescriptorPool(Device)
{
	Screen.ScreenResizeEvent([&] (int32 Width, int32 Height)
	{
		Swapchain.Free();
		Swapchain.Create(Device, Width, Height);
	});

	VkSemaphoreCreateInfo SemaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vulkan(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &ImageAvailableSem));
	vulkan(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &RenderEndSem));
}

void VulkanDRM::BeginFrame()
{
	if (VkResult Result = vkAcquireNextImageKHR(Device,
		Swapchain,
		std::numeric_limits<uint32>::max(),
		ImageAvailableSem,
		VK_NULL_HANDLE,
		&SwapchainIndex); Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		signal_unimplemented();
	}
	else
	{
		vulkan(Result);
		check(SwapchainIndex >= 0 && SwapchainIndex < Swapchain.Images.size(), "Error acquiring swapchain.");
	}
}

void VulkanDRM::EndFrame()
{
	DescriptorPool.EndFrame();
}

void VulkanDRM::SubmitCommands(drm::CommandListRef CmdList)
{ 
	const VulkanCommandListRef& VulkanCmdList = ResourceCast(CmdList);

	vulkan(vkEndCommandBuffer(VulkanCmdList->CommandBuffer));

	if (VulkanCmdList->bTouchedSurface)
	{
		const VkPipelineStageFlags WaitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = &ImageAvailableSem;
		SubmitInfo.pWaitDstStageMask = &WaitDstStageMask;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &VulkanCmdList->CommandBuffer;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &RenderEndSem;

		vulkan(vkQueueSubmit(VulkanCmdList->Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

		VkPresentInfoKHR PresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		PresentInfo.pWaitSemaphores = &RenderEndSem;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pSwapchains = &Swapchain.Swapchain;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pImageIndices = &SwapchainIndex;

		if (VkResult Result = vkQueuePresentKHR(Device.Queues.GetPresentQueue(), &PresentInfo); Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
		{
			signal_unimplemented();
		}
		else
		{
			vulkan(Result);
		}

		vulkan(vkQueueWaitIdle(Device.Queues.GetPresentQueue()));
	}
	else
	{
		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &VulkanCmdList->CommandBuffer;

		vulkan(vkQueueSubmit(VulkanCmdList->Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

		vulkan(vkQueueWaitIdle(VulkanCmdList->Queue));
	}
}

drm::CommandListRef VulkanDRM::CreateCommandList()
{
	return MakeRef<VulkanCommandList>(Device, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
}

drm::DescriptorSetRef VulkanDRM::CreateDescriptorSet()
{
	return MakeRef<VulkanDescriptorSet>(Device, DescriptorPool);
}

drm::BufferRef VulkanDRM::CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data)
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

drm::ImageRef VulkanDRM::CreateImage(uint32 Width, uint32 Height, uint32 Depth, EFormat Format, EImageUsage UsageFlags)
{
	EImageLayout Layout = EImageLayout::Undefined;

	if (drm::Image::IsDepth(Format))
	{
		Format = VulkanImage::GetEngineFormat(VulkanImage::FindSupportedDepthFormat(Device, Format));
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
	Info.initialLayout = VulkanImage::GetVulkanLayout(Layout);
	Info.usage = [&] ()
	{
		VkFlags Usage = 0;

		if (Any(UsageFlags & EImageUsage::Attachment))
		{
			Usage |= drm::Image::IsDepth(Format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

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

	VulkanImageRef VulkanImage = MakeRef<class VulkanImage>(Device
		, Image
		, Memory
		, Format
		, Layout
		, Width
		, Height
		, Depth
		, UsageFlags);

	return VulkanImage;
}

drm::ImageRef VulkanDRM::GetSurface()
{
	return Swapchain.Images[SwapchainIndex];
}

void* VulkanDRM::LockBuffer(drm::BufferRef Buffer)
{
	VulkanBufferRef VulkanBuffer = ResourceCast(Buffer);
	return Allocator.LockBuffer(*VulkanBuffer);
}

void VulkanDRM::UnlockBuffer(drm::BufferRef Buffer)
{
	VulkanBufferRef VulkanBuffer = ResourceCast(Buffer);
	Allocator.UnlockBuffer(*VulkanBuffer);
}

drm::RenderPassRef VulkanDRM::CreateRenderPass(RenderPassInitializer& RPInit)
{
	const auto[RenderPass, Framebuffer] = Device.GetRenderPass(RPInit);

	const VkRect2D RenderArea = 
	{
		RPInit.RenderArea.Offset.x,
		RPInit.RenderArea.Offset.y,
		RPInit.RenderArea.Extent.x,
		RPInit.RenderArea.Extent.y
	};

	// Get the clear values from the AttachmentViews.
	const uint32 NumRTs = RPInit.NumAttachments;
	std::vector<VkClearValue> ClearValues;

	if (RPInit.DepthAttachment.Image)
	{
		ClearValues.resize(NumRTs + 1);
	}
	else
	{
		ClearValues.resize(NumRTs);
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < NumRTs; ColorTargetIndex++)
	{
		const auto& ClearValue = std::get<ClearColorValue>(RPInit.ColorAttachments[ColorTargetIndex].ClearValue);
		memcpy(ClearValues[ColorTargetIndex].color.float32, ClearValue.Float32, sizeof(ClearValue.Float32));
		memcpy(ClearValues[ColorTargetIndex].color.int32, ClearValue.Int32, sizeof(ClearValue.Int32));
		memcpy(ClearValues[ColorTargetIndex].color.uint32, ClearValue.Uint32, sizeof(ClearValue.Uint32));
	}

	if (RPInit.DepthAttachment.Image)
	{
		VulkanImageRef Image = ResourceCast(RPInit.DepthAttachment.Image);

		ClearValues[NumRTs].depthStencil = { 0, 0 };

		if (Image->IsDepth())
		{
			ClearValues[NumRTs].depthStencil.depth = std::get<ClearDepthStencilValue>(RPInit.DepthAttachment.ClearValue).DepthClear;
		}

		if (Image->IsStencil())
		{
			ClearValues[NumRTs].depthStencil.stencil = std::get<ClearDepthStencilValue>(RPInit.DepthAttachment.ClearValue).StencilClear;
		}
	}

	return MakeRef<VulkanRenderPass>(RenderPass, Framebuffer, RenderArea, ClearValues, RPInit.NumAttachments);
}

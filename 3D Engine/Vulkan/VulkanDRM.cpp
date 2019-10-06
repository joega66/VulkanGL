#include "VulkanDRM.h"
#include "VulkanRenderTargetView.h"
#include "VulkanCommands.h"
#include "VulkanCommandList.h"
#include <Engine/Timers.h>
#include <Engine/Screen.h>

static CAST(drm::RenderTargetView, VulkanRenderTargetView);
static CAST(drm::Image, VulkanImage);
static CAST(drm::VertexBuffer, VulkanVertexBuffer);
static CAST(drm::UniformBuffer, VulkanUniformBuffer);
static CAST(drm::StorageBuffer, VulkanStorageBuffer);
static CAST(drm::IndexBuffer, VulkanIndexBuffer);
static CAST(RenderCommandList, VulkanCommandList);

HashTable<VkFormat, uint32> SizeOfVulkanFormat;
HashTable<EFormat, uint32> ImageFormatToGLSLSize;

VulkanDRM::VulkanDRM()
	: Swapchain(Device)
	, Allocator(Device)
	, DescriptorPool(Device)
{
	SizeOfVulkanFormat = ([&] ()
	{
		HashTable<VkFormat, uint32> Result; 
		for (auto&[GLSLType, Format] : GLSLTypeToVulkanFormat)
		{
			auto SizeOf = GetValue(GLSLTypeSizes, GLSLType);
			Result[Format] = SizeOf;
		} 
		return Result;
	}());

	ImageFormatToGLSLSize = ([&] ()
	{
		HashTable<EFormat, uint32> Result; 
		for (auto&[GLSLType, Format] : GLSLTypeToVulkanFormat)
		{
			auto ImageFormat = VulkanImage::GetEngineFormat(Format); 
			auto GLSLSize = GetValue(GLSLTypeSizes, GLSLType); 
			Result[ImageFormat] = GLSLSize;
		} 
		return Result;
	}());
}

void VulkanDRM::Init()
{
	Screen.RegisterScreenResChangedCallback([&](int32 Width, int32 Height)
	{
		Swapchain.Free();
		Swapchain.Init();
	});
	
	VkSemaphoreCreateInfo SemaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	vulkan(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &ImageAvailableSem));
	vulkan(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &RenderEndSem));
}

void VulkanDRM::Release()
{
	vkDestroySemaphore(Device, RenderEndSem, nullptr);
	vkDestroySemaphore(Device, ImageAvailableSem, nullptr);
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

drm::VertexBufferRef VulkanDRM::CreateVertexBuffer(EFormat EngineFormat, uint32 NumElements, EBufferUsage Usage, const void* Data)
{
	uint32 GLSLSize = GetValue(ImageFormatToGLSLSize, EngineFormat);
	auto Buffer = Allocator.CreateBuffer((VkDeviceSize)NumElements * GLSLSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanVertexBuffer>(Buffer, EngineFormat, Usage);
}

void VulkanDRM::SubmitCommands(RenderCommandListRef CmdList)
{ 
	VulkanCommandListRef VulkanCmdList = ResourceCast(CmdList);

	check(VulkanCmdList->bFinished, "Failed to call Finish() before submission.");

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
		signal_unimplemented();
	}
}

RenderCommandListRef VulkanDRM::CreateCommandList()
{
	return MakeRef<VulkanCommandList>(Device, Allocator, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
}

drm::DescriptorSetRef VulkanDRM::CreateDescriptorSet()
{
	return MakeRef<VulkanDescriptorSet>(Device, DescriptorPool, Allocator);
}

drm::IndexBufferRef VulkanDRM::CreateIndexBuffer(EFormat Format, uint32 NumIndices, EBufferUsage Usage, const void * Data)
{
	check(Format == EFormat::R16_UINT || Format == EFormat::R32_UINT, "Format must be single-channel unsigned type.");

	uint32 IndexBufferStride = GetValue(ImageFormatToGLSLSize, Format);
	auto Buffer = Allocator.CreateBuffer((VkDeviceSize)IndexBufferStride * NumIndices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanIndexBuffer>(Buffer, IndexBufferStride, Format, Usage);
}

drm::UniformBufferRef VulkanDRM::CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate UniformUsage)
{
	EBufferUsage Usage = UniformUsage == EUniformUpdate::Frequent ? EBufferUsage::KeepCPUAccessible : EBufferUsage::None;
	auto Buffer = Allocator.CreateBuffer(Size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanUniformBuffer>(Buffer);
}

drm::StorageBufferRef VulkanDRM::CreateStorageBuffer(uint32 Size, const void* Data, EBufferUsage Usage)
{
	auto Buffer = Allocator.CreateBuffer(Size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanStorageBuffer>(Buffer, Usage);
}

drm::ImageRef VulkanDRM::CreateImage(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const uint8* Data = nullptr)
{
	VkImage Image;
	VkDeviceMemory Memory;
	EImageLayout Layout;

	CreateImage(Image, Memory, Layout, Width, Height, Format, UsageFlags, Data);

	VulkanImageRef DRMImage = MakeRef<VulkanImage>(Device
		, Image
		, Memory
		, Format
		, Layout
		, Width
		, Height
		, UsageFlags);

	if (Data)
	{
		check(!Any(UsageFlags & EImageUsage::RenderTargetable), "Not supported yet.");

		TransitionImageLayout(DRMImage, 0, VK_ACCESS_TRANSFER_WRITE_BIT, EImageLayout::TransferDstOptimal, VK_PIPELINE_STAGE_TRANSFER_BIT);
		Allocator.UploadImageData(DRMImage, Data);
		TransitionImageLayout(DRMImage, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, EImageLayout::ShaderReadOnlyOptimal, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	return DRMImage;
}

drm::ImageRef VulkanDRM::CreateCubemap(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
{
	// This path will be supported, but should really prefer to use a compressed format.
	VkImage Image;
	VkDeviceMemory Memory;
	EImageLayout Layout;

	bool bHasData = std::find_if(CubemapCreateInfo.CubeFaces.begin(), CubemapCreateInfo.CubeFaces.end(),
		[] (const auto& TextureInfo) { return TextureInfo.Data; }) != CubemapCreateInfo.CubeFaces.end();

	CreateImage(Image, Memory, Layout, Width, Height, Format, UsageFlags, bHasData);

	VulkanImageRef DRMImage = MakeRef<VulkanImage>(Device
		, Image
		, Memory
		, Format
		, Layout
		, Width
		, Height
		, UsageFlags);

	if (bHasData)
	{
		TransitionImageLayout(DRMImage, 0, VK_ACCESS_TRANSFER_WRITE_BIT, EImageLayout::TransferDstOptimal, VK_PIPELINE_STAGE_TRANSFER_BIT);
		Allocator.UploadCubemapData(DRMImage, CubemapCreateInfo);
		TransitionImageLayout(DRMImage, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, EImageLayout::ShaderReadOnlyOptimal, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	return DRMImage;
}

drm::RenderTargetViewRef VulkanDRM::CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout FinalLayout)
{
	VulkanImageRef VulkanImage = ResourceCast(Image);
	VulkanRenderTargetViewRef RTView = MakeRef<VulkanRenderTargetView>(
		Device,
		VulkanImage,
		LoadAction,
		StoreAction,
		ClearValue,
		FinalLayout);
	return RTView;
}

drm::RenderTargetViewRef VulkanDRM::CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil, EImageLayout FinalLayout)
{
	VulkanImageRef VulkanImage = ResourceCast(Image);
	VulkanRenderTargetViewRef RTView = MakeRef<VulkanRenderTargetView>(
		Device,
		VulkanImage,
		LoadAction,
		StoreAction,
		DepthStencil,
		FinalLayout);
	return RTView;
}

drm::ImageRef VulkanDRM::GetSurface()
{
	return Swapchain.Images[SwapchainIndex];
}

drm::RenderTargetViewRef VulkanDRM::GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue)
{
	return MakeRef<VulkanRenderTargetView>(Device, GetSurface(), LoadAction, StoreAction, ClearValue, EImageLayout::Present);
}

void* VulkanDRM::LockBuffer(drm::VertexBufferRef VertexBuffer)
{
	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	return Allocator.LockBuffer(*VulkanVertexBuffer->Buffer);
}

void VulkanDRM::UnlockBuffer(drm::VertexBufferRef VertexBuffer)
{
	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	Allocator.UnlockBuffer(*VulkanVertexBuffer->Buffer);
}

void* VulkanDRM::LockBuffer(drm::IndexBufferRef IndexBuffer)
{
	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	return Allocator.LockBuffer(*VulkanIndexBuffer->Buffer);
}

void VulkanDRM::UnlockBuffer(drm::IndexBufferRef IndexBuffer)
{
	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	Allocator.UnlockBuffer(*VulkanIndexBuffer->Buffer);
}

void* VulkanDRM::LockBuffer(drm::StorageBufferRef StorageBuffer)
{
	VulkanStorageBufferRef VulkanStorageBuffer = ResourceCast(StorageBuffer);
	return Allocator.LockBuffer(*VulkanStorageBuffer->Buffer);
}

void VulkanDRM::UnlockBuffer(drm::StorageBufferRef StorageBuffer)
{
	VulkanStorageBufferRef VulkanStorageBuffer = ResourceCast(StorageBuffer);
	Allocator.UnlockBuffer(*VulkanStorageBuffer->Buffer);
}

void VulkanDRM::TransitionImageLayout(VulkanImageRef Image, VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask, EImageLayout NewLayout, VkPipelineStageFlags DestinationStage)
{
	VulkanScopedCommandBuffer ScopedCommandBuffer(Device, VK_QUEUE_GRAPHICS_BIT);

	VkImageMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	Barrier.oldLayout = Image->GetVulkanLayout();
	Barrier.newLayout = VulkanImage::GetVulkanLayout(NewLayout);
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = Image->Image;
	Barrier.subresourceRange.aspectMask = Image->GetVulkanAspect();
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = 0;
	Barrier.subresourceRange.layerCount = Any(Image->Usage & EImageUsage::Cubemap) ? 6 : 1;

	vkCmdPipelineBarrier(
		ScopedCommandBuffer,
		Image->Stage, DestinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &Barrier
	);

	Image->Layout = NewLayout;
	Image->Stage = DestinationStage;
}
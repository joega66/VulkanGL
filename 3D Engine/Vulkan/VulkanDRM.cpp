#include "VulkanDRM.h"
#include "VulkanRenderTargetView.h"
#include "VulkanCommands.h"
#include "VulkanCommandList.h"
#include <Engine/Timers.h>
#include <Engine/Screen.h>

static CAST(drm::RenderTargetView, VulkanRenderTargetView);
static CAST(drm::Buffer, VulkanBuffer);
static CAST(drm::Image, VulkanImage);
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
	gScreen.RegisterScreenResChangedCallback([&](int32 Width, int32 Height)
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

void VulkanDRM::SubmitCommands(RenderCommandListRef CmdList)
{ 
	const VulkanCommandListRef& VulkanCmdList = ResourceCast(CmdList);

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
		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &VulkanCmdList->CommandBuffer;

		vulkan(vkQueueSubmit(VulkanCmdList->Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

		vulkan(vkQueueWaitIdle(VulkanCmdList->Queue));
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

drm::BufferRef VulkanDRM::CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data)
{
	VkBufferUsageFlags VulkanUsage = 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Uniform) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Storage) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Index) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Vertex) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Indirect) ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;
	VulkanUsage |= !Any(Usage & EBufferUsage::KeepCPUAccessible) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;

	SharedVulkanMemoryRef Memory = Allocator.Allocate(Size, VulkanUsage, Usage, Data);

	return MakeRef<VulkanBuffer>(Memory, Usage);
}

drm::ImageRef VulkanDRM::CreateImage(uint32 Width, uint32 Height, uint32 Depth, EFormat Format, EImageUsage UsageFlags, const uint8* Data = nullptr)
{
	VkImage Image;
	VkDeviceMemory Memory;
	EImageLayout Layout;

	CreateImage(Image, Memory, Layout, Width, Height, Depth, Format, UsageFlags, Data);

	VulkanImageRef VulkanImage = MakeRef<class VulkanImage>(Device
		, Image
		, Memory
		, Format
		, Layout
		, Width
		, Height
		, Depth
		, UsageFlags);

	if (Data)
	{
		VulkanCommandListRef CmdList = ResourceCast(drm::CreateCommandList());
		ImageMemoryBarrier Barrier(VulkanImage, EAccess::NONE, EAccess::TRANSFER_WRITE, EImageLayout::TransferDstOptimal);

		CmdList->PipelineBarrier(EPipelineStage::TOP_OF_PIPE, EPipelineStage::TRANSFER, 0, nullptr, 1, &Barrier);

		// @todo CopyBufferToImage
		Allocator.UploadImageData(CmdList->CommandBuffer, VulkanImage, Data);

		Barrier.SrcAccessMask = EAccess::TRANSFER_WRITE;
		Barrier.DstAccessMask = EAccess::SHADER_READ;
		Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;

		CmdList->PipelineBarrier(EPipelineStage::TRANSFER, EPipelineStage::FRAGMENT_SHADER, 0, nullptr, 1, &Barrier);

		CmdList->Finish();

		SubmitCommands(CmdList);
	}

	if (UsageFlags == EImageUsage::Storage)
	{
		RenderCommandListRef CmdList = drm::CreateCommandList();
		ImageMemoryBarrier Barrier(VulkanImage, EAccess::NONE, EAccess::SHADER_READ | EAccess::SHADER_WRITE, EImageLayout::General);
		CmdList->PipelineBarrier(EPipelineStage::TOP_OF_PIPE, EPipelineStage::FRAGMENT_SHADER, 0, nullptr, 1, &Barrier);
		CmdList->Finish();
		SubmitCommands(CmdList);
	}

	return VulkanImage;
}

drm::ImageRef VulkanDRM::CreateCubemap(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
{
	// This path will be supported, but should really prefer to use a compressed format.
	VkImage Image;
	VkDeviceMemory Memory;
	EImageLayout Layout;

	bool bHasData = std::find_if(CubemapCreateInfo.CubeFaces.begin(), CubemapCreateInfo.CubeFaces.end(),
		[] (const auto& TextureInfo) { return TextureInfo.Data; }) != CubemapCreateInfo.CubeFaces.end();

	const uint32 Depth = 1;

	CreateImage(Image, Memory, Layout, Width, Height, Depth, Format, UsageFlags, bHasData);

	VulkanImageRef VulkanImage = MakeRef<class VulkanImage>(Device
		, Image
		, Memory
		, Format
		, Layout
		, Width
		, Height
		, Depth
		, UsageFlags);

	if (bHasData)
	{
		VulkanCommandListRef CmdList = ResourceCast(drm::CreateCommandList());
		ImageMemoryBarrier Barrier(VulkanImage, EAccess::NONE, EAccess::TRANSFER_WRITE, EImageLayout::TransferDstOptimal);

		CmdList->PipelineBarrier(EPipelineStage::TOP_OF_PIPE, EPipelineStage::TRANSFER, 0, nullptr, 1, &Barrier);

		// @todo CopyBufferToImage
		Allocator.UploadCubemapData(CmdList->CommandBuffer, VulkanImage, CubemapCreateInfo);

		Barrier.SrcAccessMask = EAccess::TRANSFER_WRITE;
		Barrier.DstAccessMask = EAccess::SHADER_READ;
		Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;

		CmdList->PipelineBarrier(EPipelineStage::TRANSFER, EPipelineStage::FRAGMENT_SHADER, 0, nullptr, 1, &Barrier);

		CmdList->Finish();

		SubmitCommands(CmdList);
	}

	return VulkanImage;
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

void* VulkanDRM::LockBuffer(drm::BufferRef Buffer)
{
	VulkanBufferRef VulkanBuffer = ResourceCast(Buffer);
	return Allocator.LockBuffer(*VulkanBuffer->Memory);
}

void VulkanDRM::UnlockBuffer(drm::BufferRef Buffer)
{
	VulkanBufferRef VulkanBuffer = ResourceCast(Buffer);
	Allocator.UnlockBuffer(*VulkanBuffer->Memory);
}
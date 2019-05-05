#include "VulkanDRM.h"
#include "VulkanRenderTargetView.h"
#include "VulkanCommands.h"
#include "VulkanCommandList.h"
#include <Engine/Timers.h>

static CAST(drm::RenderTargetView, VulkanRenderTargetView);
static CAST(drm::Image, VulkanImage);
static CAST(drm::Shader, VulkanShader);
static CAST(drm::VertexBuffer, VulkanVertexBuffer);
static CAST(drm::UniformBuffer, VulkanUniformBuffer);
static CAST(drm::StorageBuffer, VulkanStorageBuffer);
static CAST(drm::IndexBuffer, VulkanIndexBuffer);
static CAST(RenderCommandList, VulkanCommandList);

/** Engine conversions */
HashTable<VkFormat, uint32> SizeOfVulkanFormat;
HashTable<EImageFormat, uint32> ImageFormatToGLSLSize;

VulkanDRM::VulkanDRM()
	: Swapchain(Device)
	, Allocator(Device)
	, DescriptorPool(Device)
{
	SizeOfVulkanFormat = ([&] ()
	{
		HashTable<VkFormat, uint32> Result; for (auto&[GLSLType, Format] : GLSLTypeToVulkanFormat)
		{
			auto SizeOf = GetValue(GLSLTypeSizes, GLSLType);
			Result[Format] = SizeOf;
		} return Result;
	}());

	ImageFormatToGLSLSize = ([&] ()
	{
		HashTable<EImageFormat, uint32>Result; for (auto&[GLSLType, Format] : GLSLTypeToVulkanFormat)
		{
			auto ImageFormat = VulkanImage::GetEngineFormat(Format); auto GLSLSize = GetValue(GLSLTypeSizes, GLSLType); Result[ImageFormat] = GLSLSize;
		}return Result;
	}());
}

void VulkanDRM::Init()
{
	Swapchain.Init();

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
	DescriptorPool.Reset();
}

drm::VertexBufferRef VulkanDRM::CreateVertexBuffer(EImageFormat EngineFormat, uint32 NumElements, EResourceUsage Usage, const void* Data)
{
	uint32 GLSLSize = GetValue(ImageFormatToGLSLSize, EngineFormat);
	auto Buffer = Allocator.CreateBuffer(NumElements * GLSLSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanVertexBuffer>(Buffer, EngineFormat, Usage);
}

void VulkanDRM::SubmitCommands(RenderCommandListRef CmdList)
{ 
	VulkanCommandListRef VulkanCmdList = ResourceCast(CmdList);

	check(VulkanCmdList->bFinished, "Failed to call Finish() before submission.");

	if (VulkanCmdList->bTouchedSurface)
	{
		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		SubmitInfo.pWaitSemaphores = &ImageAvailableSem;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pCommandBuffers = &VulkanCmdList->CommandBuffer;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pSignalSemaphores = &RenderEndSem;
		SubmitInfo.signalSemaphoreCount = 1;

		vulkan(vkQueueSubmit(Device.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE));

		VkPresentInfoKHR PresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		PresentInfo.pWaitSemaphores = &RenderEndSem;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pSwapchains = &Swapchain.Swapchain;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pImageIndices = &SwapchainIndex;

		if (VkResult Result = vkQueuePresentKHR(Device.PresentQueue, &PresentInfo); Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
		{
			signal_unimplemented();
		}
		else
		{
			vulkan(Result);
		}
	}
	else
	{
		signal_unimplemented();
	}

	vkQueueWaitIdle(Device.PresentQueue);
}

RenderCommandListRef VulkanDRM::CreateCommandList()
{
	return MakeRef<VulkanCommandList>(Device, Allocator, DescriptorPool);
}

drm::IndexBufferRef VulkanDRM::CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void * Data)
{
	check(Format == IF_R16_UINT || Format == IF_R32_UINT, "Format must be single-channel unsigned type.");

	uint32 IndexBufferStride = GetValue(ImageFormatToGLSLSize, Format);
	auto Buffer = Allocator.CreateBuffer(IndexBufferStride * NumIndices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanIndexBuffer>(Buffer, IndexBufferStride, Format, Usage);
}

drm::UniformBufferRef VulkanDRM::CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate UniformUsage)
{
	EResourceUsage Usage = UniformUsage == EUniformUpdate::Frequent || UniformUsage == EUniformUpdate::SingleFrame
		? EResourceUsage::KeepCPUAccessible : EResourceUsage::None;
	auto Buffer = Allocator.CreateBuffer(Size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanUniformBuffer>(Buffer);
}

drm::StorageBufferRef VulkanDRM::CreateStorageBuffer(uint32 Size, const void * Data, EResourceUsage Usage)
{
	auto Buffer = Allocator.CreateBuffer(Size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanStorageBuffer>(Buffer, Usage);
}

drm::ImageRef VulkanDRM::CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data = nullptr)
{
	VkImage Image;
	VkDeviceMemory Memory;
	VkImageLayout Layout;

	CreateImage(Image, Memory, Layout, Width, Height, Format, UsageFlags, Data);

	VulkanImageRef DRMImage = MakeRef<VulkanImage>(Device
		, Image
		, Memory
		, Layout
		, Format
		, Width
		, Height
		, UsageFlags);

	if (Data)
	{
		TransitionImageLayout(DRMImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
		Allocator.UploadImageData(DRMImage, Data);
		TransitionImageLayout(DRMImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	return DRMImage;
}

drm::ImageRef VulkanDRM::CreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
{
	// This path will be supported, but should really prefer to use a compressed format.
	VkImage Image;
	VkDeviceMemory Memory;
	VkImageLayout Layout;

	bool bHasData = std::find_if(CubemapCreateInfo.CubeFaces.begin(), CubemapCreateInfo.CubeFaces.end(),
		[] (const auto& TextureInfo) { return TextureInfo.Data; }) != CubemapCreateInfo.CubeFaces.end();

	CreateImage(Image, Memory, Layout, Width, Height, Format, UsageFlags, bHasData);

	VulkanImageRef DRMImage = MakeRef<VulkanImage>(Device
		, Image
		, Memory
		, Layout
		, Format
		, Width
		, Height
		, UsageFlags);

	if (bHasData)
	{
		TransitionImageLayout(DRMImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
		Allocator.UploadCubemapData(DRMImage, CubemapCreateInfo);
		TransitionImageLayout(DRMImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	return DRMImage;
}

drm::RenderTargetViewRef VulkanDRM::CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	VulkanImageRef VulkanImage = ResourceCast(Image);
	VulkanRenderTargetViewRef RTView = MakeRef<VulkanRenderTargetView>(
		Device,
		VulkanImage,
		LoadAction,
		StoreAction,
		ClearValue);
	return RTView;
}

drm::RenderTargetViewRef VulkanDRM::CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil)
{
	VulkanImageRef VulkanImage = ResourceCast(Image);
	VulkanRenderTargetViewRef RTView = MakeRef<VulkanRenderTargetView>(
		Device,
		VulkanImage,
		LoadAction,
		StoreAction,
		DepthStencil);
	return RTView;
}

drm::ImageRef VulkanDRM::GetSurface()
{
	return Swapchain.Images[SwapchainIndex];
}

drm::RenderTargetViewRef VulkanDRM::GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	return MakeRef<VulkanRenderTargetView>(Device, GetSurface(), LoadAction, StoreAction, ClearValue);
}

void* VulkanDRM::LockBuffer(drm::VertexBufferRef VertexBuffer, uint32 Size, uint32 Offset)
{
	// @todo-joe Handle Size, Offset
	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	return Allocator.LockBuffer(*VulkanVertexBuffer->Buffer);
}

void VulkanDRM::UnlockBuffer(drm::VertexBufferRef VertexBuffer)
{
	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	Allocator.UnlockBuffer(*VulkanVertexBuffer->Buffer);
}

void* VulkanDRM::LockBuffer(drm::IndexBufferRef IndexBuffer, uint32 Size, uint32 Offset)
{
	// @todo-joe Handle Size, Offset
	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	return Allocator.LockBuffer(*VulkanIndexBuffer->Buffer);
}

void VulkanDRM::UnlockBuffer(drm::IndexBufferRef IndexBuffer)
{
	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	Allocator.UnlockBuffer(*VulkanIndexBuffer->Buffer);
}

void VulkanDRM::RebuildResolutionDependents()
{
	// @todo-joe
}

void VulkanDRM::TransitionImageLayout(VulkanImageRef Image, VkImageLayout NewLayout, VkPipelineStageFlags DestinationStage)
{
	VulkanScopedCommandBuffer ScopedCommandBuffer(Device);

	VkImageMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	Barrier.oldLayout = Image->Layout;
	Barrier.newLayout = NewLayout;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = Image->Image;
	Barrier.subresourceRange.aspectMask = Image->GetVulkanAspect();
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = 0;
	Barrier.subresourceRange.layerCount = Any(Image->Usage & EResourceUsage::Cubemap) ? 6 : 1;

	if (Image->Layout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		check(Image->Stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, "Pipeline stage is invalid for this transition.");
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (Image->Layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		check(Image->Stage == VK_PIPELINE_STAGE_TRANSFER_BIT, "Pipeline stage is invalid for this transition.");
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else
	{
		fail("Unsupported layout transition.");
	}

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

void VulkanDRM::CreateImage(VkImage& Image, VkDeviceMemory& Memory, VkImageLayout& Layout
	, uint32 Width, uint32 Height, EImageFormat& Format, EResourceUsage UsageFlags, bool bTransferDstBit)
{
	Layout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (drm::Image::IsDepth(Format))
	{
		Format = VulkanImage::GetEngineFormat(FindSupportedDepthFormat(Format));
	}

	VkImageCreateInfo Info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	Info.imageType = VK_IMAGE_TYPE_2D;
	Info.extent.width = Width;
	Info.extent.height = Height;
	Info.extent.depth = 1;
	Info.mipLevels = 1;
	Info.arrayLayers = Any(UsageFlags & EResourceUsage::Cubemap) ? 6 : 1;
	Info.format = VulkanImage::GetVulkanFormat(Format);
	Info.tiling = VK_IMAGE_TILING_OPTIMAL;
	Info.initialLayout = Layout;
	Info.usage = [&] ()
	{
		VkFlags Usage = 0;

		if (Any(UsageFlags & EResourceUsage::RenderTargetable))
		{
			Usage |= drm::Image::IsDepth(Format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		Usage |= bTransferDstBit ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		Usage |= Any(UsageFlags & EResourceUsage::ShaderResource) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
		Usage |= Any(UsageFlags & EResourceUsage::UnorderedAccess) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

		return Usage;
	}();
	Info.flags = Any(UsageFlags & EResourceUsage::Cubemap) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	Info.samples = VK_SAMPLE_COUNT_1_BIT;
	Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vulkan(vkCreateImage(Device, &Info, nullptr, &Image));

	VkMemoryRequirements MemRequirements = {};
	vkGetImageMemoryRequirements(Device, Image, &MemRequirements);

	VkMemoryAllocateInfo MemInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	MemInfo.allocationSize = MemRequirements.size;
	MemInfo.memoryTypeIndex = Allocator.FindMemoryType(MemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vulkan(vkAllocateMemory(Device, &MemInfo, nullptr, &Memory));
	vulkan(vkBindImageMemory(Device, Image, Memory, 0));
}
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanSurface.h"
#include <unordered_set>

void VulkanDevice::EndFrame()
{
	DescriptorPoolManager.EndFrame(*this);
	VulkanCache.EndFrame();
	BindlessTextures->EndFrame();
	BindlessSamplers->EndFrame();
}

void VulkanDevice::SubmitCommands(gpu::CommandList& CmdList)
{
	vulkan(vkEndCommandBuffer(CmdList._CommandBuffer));

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CmdList._CommandBuffer;

	vulkan(vkQueueSubmit(CmdList._Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

	vulkan(vkQueueWaitIdle(CmdList._Queue));
}

gpu::CommandList VulkanDevice::CreateCommandList(EQueue Queue)
{
	// @todo Transfer, AsyncCompute Queues
	const VkQueueFlags QueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
	return VulkanCommandList(*this, QueueFlags);
}

gpu::Pipeline VulkanDevice::CreatePipeline(const PipelineStateDesc& PSODesc)
{
	return VulkanCache.GetPipeline(PSODesc);
}

gpu::Pipeline VulkanDevice::CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc)
{
	return VulkanCache.GetPipeline(ComputePipelineDesc);
}

gpu::DescriptorSetLayout VulkanDevice::CreateDescriptorSetLayout(std::size_t NumEntries, const DescriptorBinding* Entries)
{
	return VulkanDescriptorSetLayout(*this, NumEntries, Entries);
}

VulkanBuffer VulkanDevice::CreateBuffer(EBufferUsage Usage, uint64 Size, const void* Data)
{
	VkBufferUsageFlags VulkanUsage = 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Indirect) ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Vertex) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Storage) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Index) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Uniform) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;
	VulkanUsage |= !Any(Usage & EBufferUsage::HostVisible) && !Any(Usage & EBufferUsage::Transfer) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;
	VulkanUsage |= Any(Usage & EBufferUsage::Transfer) ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0;

	return Allocator.Allocate(Size, VulkanUsage, Usage, Data);
}

gpu::Image VulkanDevice::CreateImage(
	uint32 Width,
	uint32 Height,
	uint32 Depth,
	EFormat Format,
	EImageUsage UsageFlags,
	uint32 MipLevels
)
{
	if (gpu::Image::IsDepth(Format))
	{
		Format = VulkanImage::GetEngineFormat(VulkanImage::FindSupportedDepthFormat(*this, Format));
	}

	VkImageCreateInfo Info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	Info.imageType = Depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	Info.extent.width = Width;
	Info.extent.height = Height;
	Info.extent.depth = Depth;
	Info.mipLevels = MipLevels;
	Info.arrayLayers = Any(UsageFlags & EImageUsage::Cubemap) ? 6 : 1;
	Info.format = VulkanImage::GetVulkanFormat(Format);
	Info.tiling = VK_IMAGE_TILING_OPTIMAL;
	Info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Info.usage = [&] ()
	{
		VkFlags Usage = 0;

		if (Any(UsageFlags & EImageUsage::Attachment))
		{
			Usage |= gpu::Image::IsDepth(Format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		Usage |= Any(UsageFlags & EImageUsage::TransferSrc) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
		Usage |= Any(UsageFlags & EImageUsage::TransferDst) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		Usage |= Any(UsageFlags & EImageUsage::Sampled) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
		Usage |= Any(UsageFlags & EImageUsage::Storage) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

		return Usage;
	}();
	Info.flags = Any(UsageFlags & EImageUsage::Cubemap) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	Info.samples = VK_SAMPLE_COUNT_1_BIT;
	Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkImage Image;
	vulkan(vkCreateImage(Device, &Info, nullptr, &Image));

	VkMemoryRequirements MemRequirements = {};
	vkGetImageMemoryRequirements(Device, Image, &MemRequirements);

	const VkDeviceMemory Memory = Allocator.AllocateMemory(MemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, MemRequirements.size);
	vulkan(vkBindImageMemory(Device, Image, Memory, 0));

	return VulkanImage(*this
		, Image
		, Memory
		, Format
		, Width
		, Height
		, Depth
		, UsageFlags
		, MipLevels
	);
}

gpu::ImageView VulkanDevice::CreateImageView(
	const gpu::Image& Image, 
	uint32 BaseMipLevel, 
	uint32 LevelCount, 
	uint32 BaseArrayLayer, 
	uint32 LayerCount
)
{
	VkImageViewCreateInfo ImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	ImageViewCreateInfo.image = Image;
	ImageViewCreateInfo.viewType = Any(Image.GetUsage() & EImageUsage::Cubemap) ? VK_IMAGE_VIEW_TYPE_CUBE : (Image.GetDepth() > 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D);
	ImageViewCreateInfo.format = Image.GetVulkanFormat();
	ImageViewCreateInfo.subresourceRange.aspectMask = Image.GetVulkanAspect();
	ImageViewCreateInfo.subresourceRange.baseMipLevel = BaseMipLevel;
	ImageViewCreateInfo.subresourceRange.levelCount = LevelCount;
	ImageViewCreateInfo.subresourceRange.baseArrayLayer = BaseArrayLayer;
	ImageViewCreateInfo.subresourceRange.layerCount = LayerCount;
	ImageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	VkImageView ImageView;
	vulkan(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &ImageView));

	return VulkanImageView(*this, ImageView, Image.GetFormat());
}

gpu::Sampler VulkanDevice::CreateSampler(const SamplerDesc& SamplerDesc)
{
	return VulkanCache.GetSampler(SamplerDesc);
}

gpu::RenderPass VulkanDevice::CreateRenderPass(const RenderPassDesc& RPDesc)
{
	const auto[RenderPass, Framebuffer] = VulkanCache.GetRenderPass(RPDesc);

	const VkRect2D RenderArea = 
	{
		RPDesc.renderArea.offset.x,
		RPDesc.renderArea.offset.y,
		RPDesc.renderArea.extent.x,
		RPDesc.renderArea.extent.y
	};

	// Get the clear values from the AttachmentViews.
	std::vector<VkClearValue> ClearValues;

	if (RPDesc.depthAttachment.image)
	{
		ClearValues.resize(RPDesc.colorAttachments.size() + 1);
	}
	else
	{
		ClearValues.resize(RPDesc.colorAttachments.size());
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < RPDesc.colorAttachments.size(); ColorTargetIndex++)
	{
		const auto& ClearValue = RPDesc.colorAttachments[ColorTargetIndex].clearColor;
		memcpy(ClearValues[ColorTargetIndex].color.float32, ClearValue.float32, sizeof(ClearValue.float32));
		memcpy(ClearValues[ColorTargetIndex].color.int32, ClearValue.int32, sizeof(ClearValue.int32));
		memcpy(ClearValues[ColorTargetIndex].color.uint32, ClearValue.uint32, sizeof(ClearValue.uint32));
	}

	if (RPDesc.depthAttachment.image)
	{
		const gpu::Image* Image = RPDesc.depthAttachment.image;

		ClearValues[RPDesc.colorAttachments.size()].depthStencil = { 0, 0 };

		if (Image->IsDepth())
		{
			ClearValues[RPDesc.colorAttachments.size()].depthStencil.depth = RPDesc.depthAttachment.clearDepthStencil.depthClear;
		}

		if (Image->IsStencil())
		{
			ClearValues[RPDesc.colorAttachments.size()].depthStencil.stencil = RPDesc.depthAttachment.clearDepthStencil.stencilClear;
		}
	}

	return VulkanRenderPass(*this, RenderPass, Framebuffer, RenderArea, ClearValues, static_cast<uint32>(RPDesc.colorAttachments.size()));
}

gpu::TextureID VulkanDevice::CreateTextureID(const VulkanImageView& ImageView)
{
	return BindlessTextures->CreateTextureID(ImageView);
}

gpu::ImageID VulkanDevice::CreateImageID(const VulkanImageView& ImageView)
{
	return BindlessImages->CreateImageID(ImageView);
}

gpu::BindlessResources& VulkanDevice::GetTextures()
{
	return *BindlessTextures;
}

gpu::BindlessResources& VulkanDevice::GetSamplers()
{
	return *BindlessSamplers;
}

gpu::BindlessResources& VulkanDevice::GetImages()
{
	return *BindlessImages;
}
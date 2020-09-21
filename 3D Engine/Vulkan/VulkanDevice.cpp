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
	return gpu::CommandList(*this, QueueFlags);
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
	return gpu::DescriptorSetLayout(*this, NumEntries, Entries);
}

gpu::Buffer VulkanDevice::CreateBuffer(EBufferUsage bufferUsage, EMemoryUsage memoryUsage, uint64 size, const void* data)
{
	VkBufferUsageFlags vulkanBufferUsage = 0;
	vulkanBufferUsage |= Any(bufferUsage & EBufferUsage::Indirect) ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;
	vulkanBufferUsage |= Any(bufferUsage & EBufferUsage::Vertex) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
	vulkanBufferUsage |= Any(bufferUsage & EBufferUsage::Storage) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
	vulkanBufferUsage |= Any(bufferUsage & EBufferUsage::Index) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
	vulkanBufferUsage |= Any(bufferUsage & EBufferUsage::Uniform) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;

	VmaAllocationCreateInfo allocInfo = {};

	if (memoryUsage == EMemoryUsage::GPU_ONLY)
	{
		vulkanBufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	}
	else if (memoryUsage == EMemoryUsage::CPU_ONLY)
	{
		vulkanBufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	else if (memoryUsage == EMemoryUsage::CPU_TO_GPU)
	{
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	else if (memoryUsage == EMemoryUsage::GPU_TO_CPU)
	{
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	else
	{
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	}

	const VkBufferCreateInfo bufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = vulkanBufferUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
	vmaCreateBuffer(_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &allocationInfo);

	gpu::Buffer newBuffer(_Allocator, buffer, allocation, allocationInfo, size, bufferUsage);

	if (data)
	{
		Platform::Memcpy(newBuffer.GetData(), data, newBuffer.GetSize());
	}

	return newBuffer;
}

gpu::Image VulkanDevice::CreateImage(
	uint32 Width,
	uint32 Height,
	uint32 Depth,
	EFormat Format,
	EImageUsage UsageFlags,
	uint32 MipLevels)
{
	if (gpu::Image::IsDepth(Format))
	{
		Format = gpu::Image::GetEngineFormat(gpu::Image::FindSupportedDepthFormat(*this, Format));
	}

	VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = Depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = Width;
	imageInfo.extent.height = Height;
	imageInfo.extent.depth = Depth;
	imageInfo.mipLevels = MipLevels;
	imageInfo.arrayLayers = Any(UsageFlags & EImageUsage::Cubemap) ? 6 : 1;
	imageInfo.format = gpu::Image::GetVulkanFormat(Format);
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = [&] ()
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
	imageInfo.flags = Any(UsageFlags & EImageUsage::Cubemap) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VkImage image;
	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
	vmaCreateImage(_Allocator, &imageInfo, &allocInfo, &image, &allocation, &allocationInfo);
	
	return gpu::Image(
		*this
		, _Allocator
		, allocation
		, allocationInfo
		, image
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

	return gpu::ImageView(*this, ImageView, Image.GetUsage(), Image.GetFormat());
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

	return gpu::RenderPass(*this, RenderPass, Framebuffer, RenderArea, ClearValues, static_cast<uint32>(RPDesc.colorAttachments.size()));
}

gpu::TextureID VulkanDevice::CreateTextureID(const gpu::ImageView& ImageView)
{
	return BindlessTextures->CreateTextureID(ImageView);
}

gpu::ImageID VulkanDevice::CreateImageID(const gpu::ImageView& ImageView)
{
	return BindlessImages->CreateImageID(ImageView);
}

gpu::BindlessDescriptors& VulkanDevice::GetTextures()
{
	return *BindlessTextures;
}

gpu::BindlessDescriptors& VulkanDevice::GetSamplers()
{
	return *BindlessSamplers;
}

gpu::BindlessDescriptors& VulkanDevice::GetImages()
{
	return *BindlessImages;
}
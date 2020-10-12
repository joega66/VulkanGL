#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include <unordered_set>

void VulkanDevice::EndFrame()
{
	_VulkanCache.EndFrame();
	_DescriptorPoolManager.EndFrame(*this);
	_BindlessTextures->EndFrame();
	_BindlessSamplers->EndFrame();
	_BindlessImages->EndFrame();
}

void VulkanDevice::SubmitCommands(gpu::CommandList& cmdList)
{
	vulkan(vkEndCommandBuffer(cmdList._CommandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdList._CommandBuffer;

	vulkan(vkQueueSubmit(cmdList._Queue, 1, &submitInfo, VK_NULL_HANDLE));

	vulkan(vkQueueWaitIdle(cmdList._Queue));
}

gpu::CommandList VulkanDevice::CreateCommandList(EQueue queue)
{
	const VulkanQueue& vulkanQueue = _Queues.GetQueue(queue);

	VkCommandBufferAllocateInfo commandBufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferInfo.commandPool = vulkanQueue.commandPool;
	commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vulkan(vkAllocateCommandBuffers(_Device, &commandBufferInfo, &commandBuffer));

	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vulkan(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	return gpu::CommandList(*this, vulkanQueue.queue, vulkanQueue.commandPool, commandBuffer);
}

gpu::Pipeline VulkanDevice::CreatePipeline(const PipelineStateDesc& psoDesc)
{
	return _VulkanCache.GetPipeline(psoDesc);
}

gpu::Pipeline VulkanDevice::CreatePipeline(const ComputePipelineDesc& computePipelineDesc)
{
	return _VulkanCache.GetPipeline(computePipelineDesc);
}

gpu::DescriptorSetLayout VulkanDevice::CreateDescriptorSetLayout(std::size_t numBindings, const DescriptorBinding* bindings)
{
	return gpu::DescriptorSetLayout(*this, numBindings, bindings);
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
	uint32 width,
	uint32 height,
	uint32 depth,
	EFormat format,
	EImageUsage imageUsage,
	uint32 mipLevels)
{
	if (gpu::Image::IsDepth(format))
	{
		format = gpu::Image::GetEngineFormat(gpu::Image::FindSupportedDepthFormat(*this, format));
	}

	VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = depth;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = Any(imageUsage & EImageUsage::Cubemap) ? 6 : 1;
	imageInfo.format = gpu::Image::GetVulkanFormat(format);
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = [&] ()
	{
		VkFlags flags = 0;

		if (Any(imageUsage & EImageUsage::Attachment))
		{
			flags |= gpu::Image::IsDepth(format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		flags |= Any(imageUsage & EImageUsage::TransferSrc) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
		flags |= Any(imageUsage & EImageUsage::TransferDst) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		flags |= Any(imageUsage & EImageUsage::Sampled) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
		flags |= Any(imageUsage & EImageUsage::Storage) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

		return flags;
	}();
	imageInfo.flags = Any(imageUsage & EImageUsage::Cubemap) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
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
		, format
		, width
		, height
		, depth
		, imageUsage
		, mipLevels
	);
}

gpu::ImageView VulkanDevice::CreateImageView(
	const gpu::Image& image, 
	uint32 baseMipLevel, 
	uint32 levelCount, 
	uint32 baseArrayLayer, 
	uint32 layerCount)
{
	VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewInfo.image = image;
	imageViewInfo.viewType = Any(image.GetUsage() & EImageUsage::Cubemap) ? VK_IMAGE_VIEW_TYPE_CUBE : (image.GetDepth() > 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D);
	imageViewInfo.format = image.GetVulkanFormat();
	imageViewInfo.subresourceRange.aspectMask = image.GetVulkanAspect();
	imageViewInfo.subresourceRange.baseMipLevel = baseMipLevel;
	imageViewInfo.subresourceRange.levelCount = levelCount;
	imageViewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
	imageViewInfo.subresourceRange.layerCount = layerCount;
	imageViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	VkImageView imageView;
	vulkan(vkCreateImageView(_Device, &imageViewInfo, nullptr, &imageView));

	return gpu::ImageView(*this, imageView, image.GetUsage(), image.GetFormat());
}

gpu::Sampler VulkanDevice::CreateSampler(const SamplerDesc& samplerDesc)
{
	return _VulkanCache.GetSampler(samplerDesc);
}

gpu::RenderPass VulkanDevice::CreateRenderPass(const RenderPassDesc& rpDesc)
{
	const auto[renderPass, framebuffer] = _VulkanCache.GetRenderPass(rpDesc);

	const VkRect2D renderArea = 
	{
		rpDesc.renderArea.offset.x,
		rpDesc.renderArea.offset.y,
		rpDesc.renderArea.extent.x,
		rpDesc.renderArea.extent.y
	};

	// Get the clear values from the AttachmentViews.
	std::vector<VkClearValue> clearValues;

	if (rpDesc.depthAttachment.image)
	{
		clearValues.resize(rpDesc.colorAttachments.size() + 1);
	}
	else
	{
		clearValues.resize(rpDesc.colorAttachments.size());
	}

	for (std::size_t i = 0; i < rpDesc.colorAttachments.size(); i++)
	{
		std::visit([&] (auto&& arg)
		{
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, std::array<float, 4>>)
			{
				memcpy(clearValues[i].color.float32, arg.data(), sizeof(clearValues[i].color.float32));
			}
			else if constexpr (std::is_same_v<T, std::array<int32, 4>>)
			{
				memcpy(clearValues[i].color.int32, arg.data(), sizeof(clearValues[i].color.int32));
			}
			else if constexpr (std::is_same_v<T, std::array<uint32, 4>>)
			{
				memcpy(clearValues[i].color.uint32, arg.data(), sizeof(clearValues[i].color.uint32));
			}
		}, rpDesc.colorAttachments[i].clearColor);
	}

	if (rpDesc.depthAttachment.image)
	{
		const gpu::Image* image = rpDesc.depthAttachment.image;

		clearValues[rpDesc.colorAttachments.size()].depthStencil = { 0, 0 };

		if (image->IsDepth())
		{
			clearValues[rpDesc.colorAttachments.size()].depthStencil.depth = rpDesc.depthAttachment.clearDepthStencil.depthClear;
		}

		if (image->IsStencil())
		{
			clearValues[rpDesc.colorAttachments.size()].depthStencil.stencil = rpDesc.depthAttachment.clearDepthStencil.stencilClear;
		}
	}

	return gpu::RenderPass(*this, renderPass, framebuffer, renderArea, clearValues, static_cast<uint32>(rpDesc.colorAttachments.size()));
}

gpu::BindlessDescriptors& VulkanDevice::GetTextures()
{
	return *_BindlessTextures;
}

gpu::BindlessDescriptors& VulkanDevice::GetSamplers()
{
	return *_BindlessSamplers;
}

gpu::BindlessDescriptors& VulkanDevice::GetImages()
{
	return *_BindlessImages;
}

const char* VulkanDevice::GetErrorString(VkResult result)
{
	const char* errorString = nullptr;
	switch (result)
	{
	case VK_NOT_READY:
		errorString = "VK_NOT_READY";
		break;
	case VK_TIMEOUT:
		errorString = "VK_TIMEOUT";
		break;
	case VK_EVENT_SET:
		errorString = "VK_EVENT_SET";
		break;
	case VK_EVENT_RESET:
		errorString = "VK_EVENT_RESET";
		break;
	case VK_INCOMPLETE:
		errorString = "VK_INCOMPLETE";
		break;
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		errorString = "VK_ERROR_OUT_OF_HOST_MEMORY";
		break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		errorString = "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		break;
	case VK_ERROR_INITIALIZATION_FAILED:
		errorString = "VK_ERROR_INITIALIZATION_FAILED";
		break;
	case VK_ERROR_DEVICE_LOST:
		errorString = "VK_ERROR_DEVICE_LOST";
		break;
	case VK_ERROR_MEMORY_MAP_FAILED:
		errorString = "VK_ERROR_MEMORY_MAP_FAILED";
		break;
	case VK_ERROR_LAYER_NOT_PRESENT:
		errorString = "VK_ERROR_LAYER_NOT_PRESENT";
		break;
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		errorString = "VK_ERROR_EXTENSION_NOT_PRESENT";
		break;
	case VK_ERROR_FEATURE_NOT_PRESENT:
		errorString = "VK_ERROR_FEATURE_NOT_PRESENT";
		break;
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		errorString = "VK_ERROR_INCOMPATIBLE_DRIVER";
		break;
	case VK_ERROR_TOO_MANY_OBJECTS:
		errorString = "VK_ERROR_TOO_MANY_OBJECTS";
		break;
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		errorString = "VK_ERROR_FORMAT_NOT_SUPPORTED";
		break;
	case VK_ERROR_FRAGMENTED_POOL:
		errorString = "VK_ERROR_FRAGMENTED_POOL";
		break;
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		errorString = "VK_ERROR_OUT_OF_POOL_MEMORY";
		break;
	case VK_ERROR_INVALID_EXTERNAL_HANDLE:
		errorString = "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		break;
	case VK_ERROR_SURFACE_LOST_KHR:
		errorString = "VK_ERROR_SURFACE_LOST_KHR";
		break;
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		errorString = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		break;
	case VK_SUBOPTIMAL_KHR:
		errorString = "VK_SUBOPTIMAL_KHR";
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
		errorString = "VK_ERROR_OUT_OF_DATE_KHR";
		break;
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		errorString = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		break;
	}
	return errorString;
}
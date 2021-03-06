#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include <unordered_set>

void VulkanDevice::EndFrame()
{
	_BindlessTextures->EndFrame();
	_BindlessImages->EndFrame();
}

void VulkanDevice::SubmitCommands(gpu::CommandBuffer& cmdBuf)
{
	SubmitCommands(cmdBuf, gpu::Semaphore(), gpu::Semaphore());
}

void VulkanDevice::SubmitCommands(
	gpu::CommandBuffer& cmdBuf, 
	const gpu::Semaphore& waitSemaphore, 
	const gpu::Semaphore& signalSemaphore)
{
	if (cmdBuf._Queue.GetQueue() == _GraphicsQueue.GetQueue())
	{
		_TransferQueue.WaitSemaphores(_Device);
	}

	cmdBuf._Queue.Submit(cmdBuf, waitSemaphore, signalSemaphore);
}

gpu::CommandBuffer VulkanDevice::CreateCommandBuffer(EQueue queueType)
{
	VulkanQueue& queue = queueType == EQueue::Transfer ? _TransferQueue : _GraphicsQueue;

	return gpu::CommandBuffer(*this, queue);
}

gpu::Pipeline VulkanDevice::CreatePipeline(const GraphicsPipelineDesc& graphicsDesc)
{
	const uint64 renderPass = *reinterpret_cast<uint64*>(graphicsDesc.renderPass.GetRenderPass());

	int i = 0;
	std::array<uint64, 5> shaders({ 0 });
	auto addShader = [&] (const gpu::Shader* shader) { if (shader) shaders[i++] = *reinterpret_cast<uint64*>(shader->compilationResult.shaderModule); };
	addShader(graphicsDesc.shaderStages.vertex);
	addShader(graphicsDesc.shaderStages.tessControl);
	addShader(graphicsDesc.shaderStages.tessEval);
	addShader(graphicsDesc.shaderStages.geometry);
	addShader(graphicsDesc.shaderStages.fragment);

	Crc crc = 0;
	Platform::crc32_u32(crc, &renderPass, sizeof(renderPass));
	Platform::crc32_u32(crc, &graphicsDesc.depthStencilState, sizeof(graphicsDesc.depthStencilState));
	Platform::crc32_u32(crc, &graphicsDesc.rasterizationState, sizeof(graphicsDesc.rasterizationState));
	Platform::crc32_u32(crc, &graphicsDesc.multisampleState, sizeof(graphicsDesc.multisampleState));
	Platform::crc32_u32(crc, &graphicsDesc.inputAssemblyState, sizeof(graphicsDesc.inputAssemblyState));
	Platform::crc32_u32(crc, shaders.data(), shaders.size() * sizeof(shaders[0]));
	Platform::crc32_u32(crc, graphicsDesc.specInfo.GetMapEntries().data(), graphicsDesc.specInfo.GetMapEntries().size() * sizeof(graphicsDesc.specInfo.GetMapEntries()[0]));
	Platform::crc32_u8(crc, graphicsDesc.specInfo.GetData().data(), graphicsDesc.specInfo.GetData().size() * sizeof(graphicsDesc.specInfo.GetData()[0]));
	Platform::crc32_u32(crc, graphicsDesc.colorBlendAttachmentStates.data(), graphicsDesc.colorBlendAttachmentStates.size() * sizeof(graphicsDesc.colorBlendAttachmentStates[0]));
	Platform::crc32_u32(crc, graphicsDesc.vertexAttributes.data(), graphicsDesc.vertexAttributes.size() * sizeof(graphicsDesc.vertexAttributes[0]));
	Platform::crc32_u32(crc, graphicsDesc.vertexBindings.data(), graphicsDesc.vertexBindings.size() * sizeof(graphicsDesc.vertexBindings[0]));

	if (auto iter = _GraphicsPipelineCache.find(crc); iter == _GraphicsPipelineCache.end())
	{
		std::map<uint32, VkDescriptorSetLayout> layoutsMap;
		auto getLayouts = [&] (const gpu::Shader* shader) 
			{ if (shader) { for (const auto& [set, layout] : shader->compilationResult.layouts) { layoutsMap.insert({ set, layout }); } } };
		getLayouts(graphicsDesc.shaderStages.vertex);
		getLayouts(graphicsDesc.shaderStages.tessControl);
		getLayouts(graphicsDesc.shaderStages.tessEval);
		getLayouts(graphicsDesc.shaderStages.geometry);
		getLayouts(graphicsDesc.shaderStages.fragment);

		std::vector<VkDescriptorSetLayout> layouts;
		layouts.reserve(layoutsMap.size());
		for (const auto& [set, layout] : layoutsMap)
		{
			layouts.push_back(layout);
		}

		std::vector<VkPushConstantRange> pushConstantRanges;
		auto getPushConstantRange = [&] (const gpu::Shader* shader) 
		{ if (shader && shader->compilationResult.pushConstantRange.size > 0) { pushConstantRanges.push_back(shader->compilationResult.pushConstantRange); } };
		getPushConstantRange(graphicsDesc.shaderStages.vertex);
		getPushConstantRange(graphicsDesc.shaderStages.tessControl);
		getPushConstantRange(graphicsDesc.shaderStages.tessEval);
		getPushConstantRange(graphicsDesc.shaderStages.geometry);
		getPushConstantRange(graphicsDesc.shaderStages.fragment);

		const VkPipelineLayout pipelineLayout = GetOrCreatePipelineLayout(layouts, pushConstantRanges);
		const gpu::Pipeline pipeline = gpu::Pipeline(std::make_shared<VkPipeline>(CreatePipeline(graphicsDesc, pipelineLayout)), pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);

		_GraphicsPipelineCache[crc] = pipeline;
		_CrcToGraphicsPipelineDesc[crc] = graphicsDesc;

		return pipeline;
	}
	else
	{
		return iter->second;
	}
}

gpu::Pipeline VulkanDevice::CreatePipeline(const ComputePipelineDesc& computeDesc)
{
	const uint64 shader = *reinterpret_cast<uint64*>(computeDesc.shader->compilationResult.shaderModule);

	Crc crc = 0;
	Platform::crc32_u32(crc, &shader, sizeof(shader));
	Platform::crc32_u32(crc, computeDesc.specInfo.GetMapEntries().data(), computeDesc.specInfo.GetMapEntries().size() * sizeof(computeDesc.specInfo.GetMapEntries()[0]));
	Platform::crc32_u8(crc, computeDesc.specInfo.GetData().data(), computeDesc.specInfo.GetData().size());

	if (auto iter = _ComputePipelineCache.find(crc); iter == _ComputePipelineCache.end())
	{
		std::vector<VkDescriptorSetLayout> layouts;
		layouts.reserve(computeDesc.shader->compilationResult.layouts.size());
		for (auto& [set, layout] : computeDesc.shader->compilationResult.layouts)
		{
			layouts.push_back(layout);
		}

		const auto& pushConstantRange = computeDesc.shader->compilationResult.pushConstantRange;
		const auto pushConstantRanges = pushConstantRange.size > 0 ? std::vector{ pushConstantRange } : std::vector<VkPushConstantRange>{};

		const VkPipelineLayout pipelineLayout = GetOrCreatePipelineLayout(layouts, pushConstantRanges);
		const gpu::Pipeline pipeline = gpu::Pipeline(std::make_shared<VkPipeline>(CreatePipeline(computeDesc, pipelineLayout)), pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE);

		_ComputePipelineCache[crc] = pipeline;
		_CrcToComputeDesc[crc] = computeDesc;

		return pipeline;
	}
	else
	{
		return iter->second;
	}
}

gpu::Buffer VulkanDevice::CreateBuffer(EBufferUsage bufferUsage, EMemoryUsage memoryUsage, uint64 size, const void* data)
{
	VkBufferUsageFlags usage = 0;
	usage |= Any(bufferUsage & EBufferUsage::Indirect) ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;
	usage |= Any(bufferUsage & EBufferUsage::Vertex) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
	usage |= Any(bufferUsage & EBufferUsage::Storage) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
	usage |= Any(bufferUsage & EBufferUsage::Index) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
	usage |= Any(bufferUsage & EBufferUsage::Uniform) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;

	VmaAllocationCreateInfo allocInfo = {};

	if (memoryUsage == EMemoryUsage::GPU_ONLY)
	{
		usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	}
	else if (memoryUsage == EMemoryUsage::CPU_ONLY)
	{
		usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
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
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
	
	vulkan( vmaCreateBuffer(_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &allocationInfo) );

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

	vulkan( vmaCreateImage(_Allocator, &imageInfo, &allocInfo, &image, &allocation, &allocationInfo) );
	
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
	Crc crc = 0;
	Platform::crc32_u32(crc, &samplerDesc, sizeof(samplerDesc));

	if (auto iter = _SamplerCache.find(crc); iter == _SamplerCache.end())
	{
		// Cache miss... Create a new sampler.
		const auto sampler = _SamplerCache.emplace(crc, gpu::Sampler(*this, samplerDesc));
		return sampler.first->second;
	}
	else
	{
		return iter->second;
	}
}

gpu::RenderPass VulkanDevice::CreateRenderPass(const RenderPassDesc& rpDesc)
{
	const auto[renderPass, framebuffer] = GetOrCreateRenderPass(rpDesc);

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

VkDescriptorSet& VulkanDevice::GetTextures()
{
	return *_BindlessTextures;
}

VkDescriptorSet& VulkanDevice::GetImages()
{
	return *_BindlessImages;
}

gpu::Semaphore VulkanDevice::CreateSemaphore()
{
	return gpu::Semaphore(_Device);
}

void VulkanDevice::UpdateDescriptorSet(
	VkDescriptorSet descriptorSet,
	VkDescriptorUpdateTemplate descriptorUpdateTemplate,
	const void* data)
{
	_VkUpdateDescriptorSetWithTemplateKHR(_Device, descriptorSet, descriptorUpdateTemplate, data);
}

void VulkanDevice::CreateDescriptorSetLayout(
	std::size_t numBindings,
	const VkDescriptorSetLayoutBinding* bindings,
	VkDescriptorSetLayout& descriptorSetLayout,
	VkDescriptorUpdateTemplate& descriptorUpdateTemplate)
{
	std::vector<VkDescriptorUpdateTemplateEntry> descriptorUpdateTemplateEntries;
	descriptorUpdateTemplateEntries.reserve(numBindings);

	uint32 structSize = 0;

	for (std::size_t bindingIndex = 0; bindingIndex < numBindings; bindingIndex++)
	{
		const auto& binding = bindings[bindingIndex];
		VkDescriptorUpdateTemplateEntry descriptorUpdateTemplateEntry = {};
		descriptorUpdateTemplateEntry.dstBinding = binding.binding;
		descriptorUpdateTemplateEntry.descriptorCount = binding.descriptorCount;
		descriptorUpdateTemplateEntry.descriptorType = binding.descriptorType;
		descriptorUpdateTemplateEntry.offset = structSize;

		structSize += 
			descriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || 
			descriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ?
			sizeof(VkDescriptorBufferInfo) : sizeof(VkDescriptorImageInfo);

		descriptorUpdateTemplateEntries.push_back(descriptorUpdateTemplateEntry);
	}

	Crc crc = 0;
	Platform::crc32_u32(crc, bindings, numBindings * sizeof(bindings[0]));

	if (auto iter = _DescriptorSetLayoutCache.find(crc); iter == _DescriptorSetLayoutCache.end())
	{
		// Cache miss... Create a new descriptor set layout.
		const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = static_cast<uint32>(numBindings),
			.pBindings = bindings
		};

		vulkan(vkCreateDescriptorSetLayout(_Device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));

		const VkDescriptorUpdateTemplateCreateInfo descriptorUpdateTemplateInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,
			.descriptorUpdateEntryCount = static_cast<uint32>(descriptorUpdateTemplateEntries.size()),
			.pDescriptorUpdateEntries = descriptorUpdateTemplateEntries.data(),
			.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET,
			.descriptorSetLayout = descriptorSetLayout,
		};

		_VkCreateDescriptorUpdateTemplateKHR(_Device, &descriptorUpdateTemplateInfo, nullptr, &descriptorUpdateTemplate);

		_DescriptorSetLayoutCache[crc] = { descriptorSetLayout, descriptorUpdateTemplate };
	}
	else
	{
		std::tie(descriptorSetLayout, descriptorUpdateTemplate) = iter->second;
	}
}

void VulkanDevice::RecompilePipelines()
{
	for (auto& [crc, pipeline] : _GraphicsPipelineCache)
	{
		vkDestroyPipeline(_Device, *pipeline._Pipeline, nullptr);
		*pipeline._Pipeline = CreatePipeline(_CrcToGraphicsPipelineDesc[crc], pipeline._PipelineLayout);
	}

	for (auto& [crc, pipeline] : _ComputePipelineCache)
	{
		vkDestroyPipeline(_Device, *pipeline._Pipeline, nullptr);
		*pipeline._Pipeline = CreatePipeline(_CrcToComputeDesc[crc], pipeline._PipelineLayout);
	}
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
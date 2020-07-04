#include "VulkanCommandList.h"
#include "VulkanDevice.h"
#include <GPU/GPUShader.h>
#include <GPU/GPUDefinitions.h>

VulkanCommandList::VulkanCommandList(VulkanDevice& device, VkQueueFlags queueFlags)
	: _Device(device)
	, _Queue(device.GetQueues().GetQueue(queueFlags))
	, _CommandPool(device.GetQueues().GetCommandPool(queueFlags))
{
	VkCommandBufferAllocateInfo CommandBufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	CommandBufferInfo.commandPool = _CommandPool;
	CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CommandBufferInfo.commandBufferCount = 1;

	vulkan(vkAllocateCommandBuffers(_Device, &CommandBufferInfo, &_CommandBuffer));

	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vulkan(vkBeginCommandBuffer(_CommandBuffer, &BeginInfo));
}

VulkanCommandList::~VulkanCommandList()
{
	vkFreeCommandBuffers(_Device, _CommandPool, 1, &_CommandBuffer);
}

void VulkanCommandList::BeginRenderPass(const VulkanRenderPass& renderPass)
{
	VkRenderPassBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	BeginInfo.renderPass = renderPass.GetRenderPass();
	BeginInfo.framebuffer = renderPass.GetFramebuffer();
	BeginInfo.renderArea = renderPass.GetRenderArea();
	BeginInfo.pClearValues = renderPass.GetClearValues().data();
	BeginInfo.clearValueCount = static_cast<uint32>(renderPass.GetClearValues().size());

	vkCmdBeginRenderPass(_CommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandList::EndRenderPass()
{
	vkCmdEndRenderPass(_CommandBuffer);
}

void VulkanCommandList::BindPipeline(const std::shared_ptr<VulkanPipeline>& pipeline)
{
	vkCmdBindPipeline(_CommandBuffer, pipeline->GetPipelineBindPoint(), pipeline->GetPipeline());
}

void VulkanCommandList::BindDescriptorSets(const std::shared_ptr<VulkanPipeline>& pipeline, std::size_t numDescriptorSets, const VkDescriptorSet* descriptorSets)
{
	vkCmdBindDescriptorSets(
		_CommandBuffer,
		pipeline->GetPipelineBindPoint(),
		pipeline->GetPipelineLayout(),
		0,
		static_cast<uint32>(numDescriptorSets),
		descriptorSets,
		0,
		nullptr
	);
}

static VkShaderStageFlags TranslateStageFlags(EShaderStage stageFlags)
{
	VkShaderStageFlags VkStageFlags = 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::Vertex) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::TessControl) ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::TessEvaluation) ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::Geometry) ? VK_SHADER_STAGE_GEOMETRY_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::Fragment) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::Compute) ? VK_SHADER_STAGE_COMPUTE_BIT : 0;
	return VkStageFlags;
}

void VulkanCommandList::PushConstants(
	const std::shared_ptr<VulkanPipeline>& pipeline, 
	EShaderStage stageFlags, 
	uint32 offset, 
	uint32 size, 
	const void* values)
{
	vkCmdPushConstants(
		_CommandBuffer,
		pipeline->GetPipelineLayout(),
		TranslateStageFlags(stageFlags),
		offset,
		size,
		values
	);
}

void VulkanCommandList::PushConstants(const std::shared_ptr<VulkanPipeline>& pipeline, const gpu::Shader* shader, const void* values)
{
	const auto& PushConstantRange = shader->compilationInfo.pushConstantRange;

	vkCmdPushConstants(
		_CommandBuffer,
		pipeline->GetPipelineLayout(),
		TranslateStageFlags(PushConstantRange.stageFlags),
		PushConstantRange.offset,
		PushConstantRange.size,
		values
	);
}

void VulkanCommandList::BindVertexBuffers(uint32 numVertexBuffers, const VulkanBuffer* vertexBuffers)
{
	std::vector<VkDeviceSize> Offsets(numVertexBuffers);
	std::vector<VkBuffer> Buffers(numVertexBuffers);

	for (uint32 Location = 0; Location < numVertexBuffers; Location++)
	{
		Offsets[Location] = vertexBuffers[Location].GetOffset();
		Buffers[Location] = vertexBuffers[Location].GetHandle();
	}

	vkCmdBindVertexBuffers(_CommandBuffer, 0, static_cast<uint32>(Buffers.size()), Buffers.data(), Offsets.data());
}

void VulkanCommandList::DrawIndexed(
	const VulkanBuffer& indexBuffer, 
	uint32 indexCount, 
	uint32 instanceCount, 
	uint32 firstIndex, 
	uint32 vertexOffset, 
	uint32 firstInstance,
	EIndexType indexType)
{
	vkCmdBindIndexBuffer(_CommandBuffer, indexBuffer.GetHandle(), indexBuffer.GetOffset(), static_cast<VkIndexType>(indexType));
	vkCmdDrawIndexed(_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandList::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
{
	vkCmdDraw(_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandList::DrawIndirect(const VulkanBuffer& buffer, uint32 offset, uint32 drawCount)
{
	vkCmdDrawIndirect(
		_CommandBuffer,
		buffer.GetHandle(),
		buffer.GetOffset() + offset,
		drawCount,
		drawCount > 1 ? sizeof(VkDrawIndirectCommand) : 0
	);
}

void VulkanCommandList::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
{
	vkCmdDispatch(_CommandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VulkanCommandList::ClearColorImage(const VulkanImage& image, EImageLayout imageLayout, const ClearColorValue& color)
{
	VkImageSubresourceRange Range = {};
	Range.aspectMask = image.GetVulkanAspect();
	Range.baseMipLevel = 0;
	Range.levelCount = image.GetMipLevels();
	Range.baseArrayLayer = 0;
	Range.layerCount = 1;

	vkCmdClearColorImage(_CommandBuffer, image, VulkanImage::GetVulkanLayout(imageLayout), reinterpret_cast<const VkClearColorValue*>(&color), 1, &Range);
}

void VulkanCommandList::ClearDepthStencilImage(const VulkanImage& image, EImageLayout imageLayout, const ClearDepthStencilValue& depthStencilValue)
{
	VkImageSubresourceRange Range = {};
	Range.aspectMask = image.GetVulkanAspect();
	Range.baseMipLevel = 0;
	Range.levelCount = 1;
	Range.baseArrayLayer = 0;
	Range.layerCount = 1;
	
	vkCmdClearDepthStencilImage(_CommandBuffer, image, VulkanImage::GetVulkanLayout(imageLayout), reinterpret_cast<const VkClearDepthStencilValue*>(&depthStencilValue), 1, &Range);
}

static inline VkAccessFlags GetVulkanAccessFlags(EAccess access)
{
	return VkAccessFlags(access);
}

static inline VkPipelineStageFlags GetVulkanPipelineStageFlags(EPipelineStage pipelineStage)
{
	return VkPipelineStageFlags(pipelineStage);
}

void VulkanCommandList::PipelineBarrier(
	EPipelineStage srcStageMask,
	EPipelineStage dstStageMask,
	std::size_t numBufferBarriers,
	const BufferMemoryBarrier* bufferBarriers,
	std::size_t numImageBarriers,
	const ImageMemoryBarrier* imageBarriers)
{
	std::vector<VkBufferMemoryBarrier> VulkanBufferBarriers;
	VulkanBufferBarriers.reserve(numBufferBarriers);

	for (std::size_t BarrierIndex = 0; BarrierIndex < numBufferBarriers; BarrierIndex++)
	{
		const BufferMemoryBarrier& BufferBarrier = bufferBarriers[BarrierIndex];
		const VulkanBuffer& Buffer = BufferBarrier.buffer;

		VkBufferMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		Barrier.srcAccessMask = GetVulkanAccessFlags(BufferBarrier.srcAccessMask);
		Barrier.dstAccessMask = GetVulkanAccessFlags(BufferBarrier.dstAccessMask);
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.buffer = Buffer.GetHandle();
		Barrier.offset = Buffer.GetOffset();
		Barrier.size = Buffer.GetSize();

		VulkanBufferBarriers.push_back(Barrier);
	}
	
	std::vector<VkImageMemoryBarrier> VulkanImageBarriers;
	VulkanImageBarriers.reserve(numImageBarriers);

	for (std::size_t BarrierIndex = 0; BarrierIndex < numImageBarriers; BarrierIndex++)
	{
		const ImageMemoryBarrier& ImageBarrier = imageBarriers[BarrierIndex];
		const gpu::Image& Image = ImageBarrier.image;

		VkImageMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		Barrier.srcAccessMask = GetVulkanAccessFlags(ImageBarrier.srcAccessMask);
		Barrier.dstAccessMask = GetVulkanAccessFlags(ImageBarrier.dstAccessMask);
		Barrier.oldLayout = VulkanImage::GetVulkanLayout(ImageBarrier.oldLayout);
		Barrier.newLayout = VulkanImage::GetVulkanLayout(ImageBarrier.newLayout);
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.image = Image;
		Barrier.subresourceRange.aspectMask = Image.GetVulkanAspect();
		Barrier.subresourceRange.baseMipLevel = ImageBarrier.baseMipLevel;
		Barrier.subresourceRange.levelCount = ImageBarrier.levelCount;
		Barrier.subresourceRange.baseArrayLayer = 0;
		Barrier.subresourceRange.layerCount = Any(Image.GetUsage() & EImageUsage::Cubemap) ? 6 : 1;

		VulkanImageBarriers.push_back(Barrier);
	}
	
	vkCmdPipelineBarrier(
		_CommandBuffer,
		GetVulkanPipelineStageFlags(srcStageMask),
		GetVulkanPipelineStageFlags(dstStageMask),
		0,
		0, nullptr,
		static_cast<uint32>(VulkanBufferBarriers.size()), VulkanBufferBarriers.data(),
		static_cast<uint32>(VulkanImageBarriers.size()), VulkanImageBarriers.data()
	);
}

void VulkanCommandList::CopyBufferToImage(
	const VulkanBuffer& srcBuffer, 
	uint32 bufferOffset, 
	const VulkanImage& dstImage, 
	EImageLayout dstImageLayout
)
{
	std::vector<VkBufferImageCopy> Regions;

	if (Any(dstImage.GetUsage() & EImageUsage::Cubemap))
	{
		const uint32 FaceSize = dstImage.GetSize() / 6;

		Regions.resize(6, {});

		for (uint32 LayerIndex = 0; LayerIndex < Regions.size(); LayerIndex++)
		{
			// VkImageSubresourceRange(3) Manual Page:
			// "...the layers of the image view starting at baseArrayLayer correspond to faces in the order +X, -X, +Y, -Y, +Z, -Z"
			VkBufferImageCopy& Region = Regions[LayerIndex];
			Region.bufferOffset = LayerIndex * FaceSize + srcBuffer.GetOffset();
			Region.bufferRowLength = 0;
			Region.bufferImageHeight = 0;
			Region.imageSubresource.aspectMask = dstImage.GetVulkanAspect();
			Region.imageSubresource.mipLevel = 0;
			Region.imageSubresource.baseArrayLayer = LayerIndex;
			Region.imageSubresource.layerCount = 1;
			Region.imageOffset = { 0, 0, 0 };
			Region.imageExtent = {
				dstImage.GetWidth(),
				dstImage.GetHeight(),
				dstImage.GetDepth()
			};
		}
	}
	else
	{
		VkBufferImageCopy Region = {};
		Region.bufferOffset = bufferOffset + srcBuffer.GetOffset();
		Region.bufferRowLength = 0;
		Region.bufferImageHeight = 0;
		Region.imageSubresource.aspectMask = dstImage.GetVulkanAspect();
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;
		Region.imageOffset = { 0, 0, 0 };
		Region.imageExtent = {
			dstImage.GetWidth(),
			dstImage.GetHeight(),
			dstImage.GetDepth()
		};

		Regions.push_back(Region);
	}

	vkCmdCopyBufferToImage(_CommandBuffer, srcBuffer.GetHandle(), dstImage, VulkanImage::GetVulkanLayout(dstImageLayout), static_cast<uint32>(Regions.size()), Regions.data());
}

void VulkanCommandList::BlitImage(
	const VulkanImage& srcImage,
	EImageLayout srcImageLayout,
	const VulkanImage& dstImage,
	EImageLayout dstImageLayout,
	EFilter filter
)
{
	VkImageBlit Region = {};
	Region.srcSubresource.aspectMask = srcImage.GetVulkanAspect();
	Region.srcSubresource.mipLevel = 0;
	Region.srcSubresource.baseArrayLayer = 0;
	Region.srcSubresource.layerCount = 1;
	Region.dstSubresource.aspectMask = dstImage.GetVulkanAspect();
	Region.dstSubresource.mipLevel = 0;
	Region.dstSubresource.baseArrayLayer = 0;
	Region.dstSubresource.layerCount = 1;
	Region.srcOffsets[1].x = srcImage.GetWidth();
	Region.srcOffsets[1].y = srcImage.GetHeight();
	Region.srcOffsets[1].z = 1;
	Region.dstOffsets[1].x = dstImage.GetWidth();
	Region.dstOffsets[1].y = dstImage.GetHeight();
	Region.dstOffsets[1].z = 1;

	vkCmdBlitImage(
		_CommandBuffer,
		srcImage,
		VulkanImage::GetVulkanLayout(srcImageLayout),
		dstImage,
		VulkanImage::GetVulkanLayout(dstImageLayout),
		1,
		&Region,
		VulkanImage::GetVulkanFilter(filter)
	);
}

void VulkanCommandList::SetScissor(uint32 scissorCount, const Scissor* scissors)
{
	static_assert(sizeof(Scissor) == sizeof(VkRect2D));
	vkCmdSetScissor(_CommandBuffer, 0, scissorCount, reinterpret_cast<const VkRect2D*>(scissors));
}

void VulkanCommandList::CopyBuffer(
	const VulkanBuffer& srcBuffer, 
	const VulkanBuffer& dstBuffer, 
	uint64 srcOffset, 
	uint64 dstOffset, 
	uint64 size
)
{
	const VkBufferCopy Region = 
	{ 
		srcBuffer.GetOffset() + srcOffset, 
		dstBuffer.GetOffset() + dstOffset,
		size
	};

	vkCmdCopyBuffer(_CommandBuffer, srcBuffer.GetHandle(), dstBuffer.GetHandle(), 1, &Region);
}

void VulkanCommandList::CopyImage(
	const VulkanImage& srcImage, 
	EImageLayout srcImageLayout, 
	const VulkanImage& dstImage, 
	EImageLayout dstImageLayout,
	uint32 dstArrayLayer
)
{
	VkImageCopy Region = {};

	Region.srcSubresource.aspectMask = srcImage.GetVulkanAspect();
	Region.srcSubresource.mipLevel = 0;
	Region.srcSubresource.baseArrayLayer = 0;
	Region.srcSubresource.layerCount = 1;

	Region.dstSubresource.aspectMask = dstImage.GetVulkanAspect();
	Region.dstSubresource.mipLevel = 0;
	Region.dstSubresource.baseArrayLayer = dstArrayLayer;
	Region.dstSubresource.layerCount = 1;

	Region.extent.width = srcImage.GetWidth();
	Region.extent.height = srcImage.GetHeight();
	Region.extent.depth = 1;

	vkCmdCopyImage(
		_CommandBuffer,
		srcImage,
		VulkanImage::GetVulkanLayout(srcImageLayout),
		dstImage,
		VulkanImage::GetVulkanLayout(dstImageLayout),
		1,
		&Region
	);
}

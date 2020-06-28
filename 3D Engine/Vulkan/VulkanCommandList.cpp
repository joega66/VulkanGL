#include "VulkanCommandList.h"
#include "VulkanDevice.h"
#include <DRMShader.h>
#include <DRMDefinitions.h>

VulkanCommandList::VulkanCommandList(VulkanDevice& Device, VkQueueFlags QueueFlags)
	: Device(Device)
	, Queue(Device.GetQueues().GetQueue(QueueFlags))
	, CommandPool(Device.GetQueues().GetCommandPool(QueueFlags))
{
	VkCommandBufferAllocateInfo CommandBufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	CommandBufferInfo.commandPool = CommandPool;
	CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CommandBufferInfo.commandBufferCount = 1;

	vulkan(vkAllocateCommandBuffers(Device, &CommandBufferInfo, &CommandBuffer));

	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vulkan(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));
}

VulkanCommandList::~VulkanCommandList()
{
	vkFreeCommandBuffers(Device, CommandPool, 1, &CommandBuffer);
}

void VulkanCommandList::BeginRenderPass(const VulkanRenderPass& RenderPass)
{
	VkRenderPassBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	BeginInfo.renderPass = RenderPass.GetRenderPass();
	BeginInfo.framebuffer = RenderPass.GetFramebuffer();
	BeginInfo.renderArea = RenderPass.GetRenderArea();
	BeginInfo.pClearValues = RenderPass.GetClearValues().data();
	BeginInfo.clearValueCount = static_cast<uint32>(RenderPass.GetClearValues().size());

	vkCmdBeginRenderPass(CommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandList::EndRenderPass()
{
	vkCmdEndRenderPass(CommandBuffer);
}

void VulkanCommandList::BindPipeline(const std::shared_ptr<VulkanPipeline>& Pipeline)
{
	vkCmdBindPipeline(CommandBuffer, Pipeline->GetPipelineBindPoint(), Pipeline->GetPipeline());
}

void VulkanCommandList::BindDescriptorSets(const std::shared_ptr<VulkanPipeline>& Pipeline, uint32 NumDescriptorSets, const VkDescriptorSet* DescriptorSets)
{
	vkCmdBindDescriptorSets(
		CommandBuffer,
		Pipeline->GetPipelineBindPoint(),
		Pipeline->GetPipelineLayout(),
		0,
		NumDescriptorSets,
		DescriptorSets,
		0,
		nullptr
	);
}

static VkShaderStageFlags TranslateStageFlags(EShaderStage StageFlags)
{
	VkShaderStageFlags VkStageFlags = 0;
	VkStageFlags |= Any(StageFlags & EShaderStage::Vertex) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
	VkStageFlags |= Any(StageFlags & EShaderStage::TessControl) ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0;
	VkStageFlags |= Any(StageFlags & EShaderStage::TessEvaluation) ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0;
	VkStageFlags |= Any(StageFlags & EShaderStage::Geometry) ? VK_SHADER_STAGE_GEOMETRY_BIT : 0;
	VkStageFlags |= Any(StageFlags & EShaderStage::Fragment) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	VkStageFlags |= Any(StageFlags & EShaderStage::Compute) ? VK_SHADER_STAGE_COMPUTE_BIT : 0;
	return VkStageFlags;
}

void VulkanCommandList::PushConstants(
	const std::shared_ptr<VulkanPipeline>& Pipeline, 
	EShaderStage StageFlags, 
	uint32 Offset, 
	uint32 Size, 
	const void* Values)
{
	vkCmdPushConstants(
		CommandBuffer,
		Pipeline->GetPipelineLayout(),
		TranslateStageFlags(StageFlags),
		Offset,
		Size,
		Values
	);
}

void VulkanCommandList::PushConstants(const std::shared_ptr<VulkanPipeline>& Pipeline, const drm::Shader* Shader, const void* Values)
{
	const auto& PushConstantRange = Shader->compilationInfo.pushConstantRange;

	vkCmdPushConstants(
		CommandBuffer,
		Pipeline->GetPipelineLayout(),
		TranslateStageFlags(PushConstantRange.stageFlags),
		PushConstantRange.offset,
		PushConstantRange.size,
		Values
	);
}

void VulkanCommandList::BindVertexBuffers(uint32 NumVertexBuffers, const VulkanBuffer* VertexBuffers)
{
	std::vector<VkDeviceSize> Offsets(NumVertexBuffers);
	std::vector<VkBuffer> Buffers(NumVertexBuffers);

	for (uint32 Location = 0; Location < NumVertexBuffers; Location++)
	{
		Offsets[Location] = VertexBuffers[Location].GetOffset();
		Buffers[Location] = VertexBuffers[Location].GetHandle();
	}

	vkCmdBindVertexBuffers(CommandBuffer, 0, static_cast<uint32>(Buffers.size()), Buffers.data(), Offsets.data());
}

void VulkanCommandList::DrawIndexed(
	const VulkanBuffer& IndexBuffer, 
	uint32 IndexCount, 
	uint32 InstanceCount, 
	uint32 FirstIndex, 
	uint32 VertexOffset, 
	uint32 FirstInstance,
	EIndexType IndexType)
{
	vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.GetHandle(), IndexBuffer.GetOffset(), static_cast<VkIndexType>(IndexType));
	vkCmdDrawIndexed(CommandBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void VulkanCommandList::Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
{
	vkCmdDraw(CommandBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

void VulkanCommandList::DrawIndirect(const VulkanBuffer& Buffer, uint32 Offset, uint32 DrawCount)
{
	vkCmdDrawIndirect(
		CommandBuffer,
		Buffer.GetHandle(),
		Buffer.GetOffset() + Offset,
		DrawCount,
		DrawCount > 1 ? sizeof(VkDrawIndirectCommand) : 0
	);
}

void VulkanCommandList::Dispatch(uint32 GroupCountX, uint32 GroupCountY, uint32 GroupCountZ)
{
	vkCmdDispatch(CommandBuffer, GroupCountX, GroupCountY, GroupCountZ);
}

void VulkanCommandList::ClearColorImage(const VulkanImage& Image, EImageLayout ImageLayout, const ClearColorValue& Color)
{
	VkImageSubresourceRange Range = {};
	Range.aspectMask = Image.GetVulkanAspect();
	Range.baseMipLevel = 0;
	Range.levelCount = Image.GetMipLevels();
	Range.baseArrayLayer = 0;
	Range.layerCount = 1;

	vkCmdClearColorImage(CommandBuffer, Image, VulkanImage::GetVulkanLayout(ImageLayout), reinterpret_cast<const VkClearColorValue*>(&Color), 1, &Range);
}

void VulkanCommandList::ClearDepthStencilImage(const VulkanImage& Image, EImageLayout ImageLayout, const ClearDepthStencilValue& DepthStencilValue)
{
	VkImageSubresourceRange Range = {};
	Range.aspectMask = Image.GetVulkanAspect();
	Range.baseMipLevel = 0;
	Range.levelCount = 1;
	Range.baseArrayLayer = 0;
	Range.layerCount = 1;
	
	vkCmdClearDepthStencilImage(CommandBuffer, Image, VulkanImage::GetVulkanLayout(ImageLayout), reinterpret_cast<const VkClearDepthStencilValue*>(&DepthStencilValue), 1, &Range);
}

static inline VkAccessFlags GetVulkanAccessFlags(EAccess Access)
{
	return VkAccessFlags(Access);
}

static inline VkPipelineStageFlags GetVulkanPipelineStageFlags(EPipelineStage PipelineStage)
{
	return VkPipelineStageFlags(PipelineStage);
}

void VulkanCommandList::PipelineBarrier(
	EPipelineStage SrcStageMask,
	EPipelineStage DstStageMask,
	std::size_t NumBufferBarriers,
	const BufferMemoryBarrier* BufferBarriers,
	std::size_t NumImageBarriers,
	const ImageMemoryBarrier* ImageBarriers)
{
	std::vector<VkBufferMemoryBarrier> VulkanBufferBarriers;
	VulkanBufferBarriers.reserve(NumBufferBarriers);

	for (std::size_t BarrierIndex = 0; BarrierIndex < NumBufferBarriers; BarrierIndex++)
	{
		const BufferMemoryBarrier& BufferBarrier = BufferBarriers[BarrierIndex];
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
	VulkanImageBarriers.reserve(NumImageBarriers);

	for (std::size_t BarrierIndex = 0; BarrierIndex < NumImageBarriers; BarrierIndex++)
	{
		const ImageMemoryBarrier& ImageBarrier = ImageBarriers[BarrierIndex];
		const drm::Image& Image = ImageBarrier.image;

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
		CommandBuffer,
		GetVulkanPipelineStageFlags(SrcStageMask),
		GetVulkanPipelineStageFlags(DstStageMask),
		0,
		0, nullptr,
		static_cast<uint32>(VulkanBufferBarriers.size()), VulkanBufferBarriers.data(),
		static_cast<uint32>(VulkanImageBarriers.size()), VulkanImageBarriers.data()
	);
}

void VulkanCommandList::CopyBufferToImage(
	const VulkanBuffer& SrcBuffer, 
	uint32 BufferOffset, 
	const VulkanImage& DstImage, 
	EImageLayout DstImageLayout
)
{
	std::vector<VkBufferImageCopy> Regions;

	if (Any(DstImage.GetUsage() & EImageUsage::Cubemap))
	{
		const uint32 FaceSize = DstImage.GetSize() / 6;

		Regions.resize(6, {});

		for (uint32 LayerIndex = 0; LayerIndex < Regions.size(); LayerIndex++)
		{
			// VkImageSubresourceRange(3) Manual Page:
			// "...the layers of the image view starting at baseArrayLayer correspond to faces in the order +X, -X, +Y, -Y, +Z, -Z"
			VkBufferImageCopy& Region = Regions[LayerIndex];
			Region.bufferOffset = LayerIndex * FaceSize + SrcBuffer.GetOffset();
			Region.bufferRowLength = 0;
			Region.bufferImageHeight = 0;
			Region.imageSubresource.aspectMask = DstImage.GetVulkanAspect();
			Region.imageSubresource.mipLevel = 0;
			Region.imageSubresource.baseArrayLayer = LayerIndex;
			Region.imageSubresource.layerCount = 1;
			Region.imageOffset = { 0, 0, 0 };
			Region.imageExtent = {
				DstImage.GetWidth(),
				DstImage.GetHeight(),
				DstImage.GetDepth()
			};
		}
	}
	else
	{
		VkBufferImageCopy Region = {};
		Region.bufferOffset = BufferOffset + SrcBuffer.GetOffset();
		Region.bufferRowLength = 0;
		Region.bufferImageHeight = 0;
		Region.imageSubresource.aspectMask = DstImage.GetVulkanAspect();
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;
		Region.imageOffset = { 0, 0, 0 };
		Region.imageExtent = {
			DstImage.GetWidth(),
			DstImage.GetHeight(),
			DstImage.GetDepth()
		};

		Regions.push_back(Region);
	}

	vkCmdCopyBufferToImage(CommandBuffer, SrcBuffer.GetHandle(), DstImage, VulkanImage::GetVulkanLayout(DstImageLayout), static_cast<uint32>(Regions.size()), Regions.data());
}

void VulkanCommandList::BlitImage(
	const VulkanImage& SrcImage,
	EImageLayout SrcImageLayout,
	const VulkanImage& DstImage,
	EImageLayout DstImageLayout,
	EFilter Filter
)
{
	VkImageBlit Region = {};
	Region.srcSubresource.aspectMask = SrcImage.GetVulkanAspect();
	Region.srcSubresource.mipLevel = 0;
	Region.srcSubresource.baseArrayLayer = 0;
	Region.srcSubresource.layerCount = 1;
	Region.dstSubresource.aspectMask = DstImage.GetVulkanAspect();
	Region.dstSubresource.mipLevel = 0;
	Region.dstSubresource.baseArrayLayer = 0;
	Region.dstSubresource.layerCount = 1;
	Region.srcOffsets[1].x = SrcImage.GetWidth();
	Region.srcOffsets[1].y = SrcImage.GetHeight();
	Region.srcOffsets[1].z = 1;
	Region.dstOffsets[1].x = DstImage.GetWidth();
	Region.dstOffsets[1].y = DstImage.GetHeight();
	Region.dstOffsets[1].z = 1;

	vkCmdBlitImage(
		CommandBuffer,
		SrcImage,
		VulkanImage::GetVulkanLayout(SrcImageLayout),
		DstImage,
		VulkanImage::GetVulkanLayout(DstImageLayout),
		1,
		&Region,
		VulkanImage::GetVulkanFilter(Filter)
	);
}

void VulkanCommandList::SetScissor(uint32 ScissorCount, const Scissor* Scissors)
{
	static_assert(sizeof(Scissor) == sizeof(VkRect2D));
	vkCmdSetScissor(CommandBuffer, 0, ScissorCount, reinterpret_cast<const VkRect2D*>(Scissors));
}

void VulkanCommandList::CopyBuffer(
	const VulkanBuffer& SrcBuffer, 
	const VulkanBuffer& DstBuffer, 
	uint64 SrcOffset, 
	uint64 DstOffset, 
	uint64 Size
)
{
	const VkBufferCopy Region = 
	{ 
		SrcBuffer.GetOffset() + SrcOffset, 
		DstBuffer.GetOffset() + DstOffset,
		Size 
	};

	vkCmdCopyBuffer(CommandBuffer, SrcBuffer.GetHandle(), DstBuffer.GetHandle(), 1, &Region);
}

void VulkanCommandList::CopyImage(
	const VulkanImage& SrcImage, 
	EImageLayout SrcImageLayout, 
	const VulkanImage& DstImage, 
	EImageLayout DstImageLayout,
	uint32 DstArrayLayer
)
{
	VkImageCopy Region = {};

	Region.srcSubresource.aspectMask = SrcImage.GetVulkanAspect();
	Region.srcSubresource.mipLevel = 0;
	Region.srcSubresource.baseArrayLayer = 0;
	Region.srcSubresource.layerCount = 1;

	Region.dstSubresource.aspectMask = DstImage.GetVulkanAspect();
	Region.dstSubresource.mipLevel = 0;
	Region.dstSubresource.baseArrayLayer = DstArrayLayer;
	Region.dstSubresource.layerCount = 1;

	Region.extent.width = SrcImage.GetWidth();
	Region.extent.height = SrcImage.GetHeight();
	Region.extent.depth = 1;

	vkCmdCopyImage(
		CommandBuffer,
		SrcImage,
		VulkanImage::GetVulkanLayout(SrcImageLayout),
		DstImage,
		VulkanImage::GetVulkanLayout(DstImageLayout),
		1,
		&Region
	);
}

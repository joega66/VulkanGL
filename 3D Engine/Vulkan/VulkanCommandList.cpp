#include "VulkanCommandList.h"
#include "VulkanDevice.h"
#include <DRMShader.h>
#include <DRMDefinitions.h>

VulkanCommandList::VulkanCommandList(VulkanDevice& Device, VkQueueFlagBits QueueFlags)
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
	BeginInfo.clearValueCount = RenderPass.GetClearValues().size();

	vkCmdBeginRenderPass(CommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandList::EndRenderPass()
{
	vkCmdEndRenderPass(CommandBuffer);
}

void VulkanCommandList::BindPipeline(const VulkanPipeline& Pipeline)
{
	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.GetPipeline());
}

void VulkanCommandList::BindDescriptorSets(const VulkanPipeline& Pipeline, uint32 NumDescriptorSets, const VulkanDescriptorSet** DescriptorSets)
{
	std::vector<VkDescriptorSet> VulkanDescriptorSets(NumDescriptorSets);
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts(NumDescriptorSets);

	for (uint32 SetIndex = 0; SetIndex < NumDescriptorSets; SetIndex++)
	{
		VulkanDescriptorSets[SetIndex] = DescriptorSets[SetIndex]->GetVulkanHandle();
		DescriptorSetLayouts[SetIndex] = DescriptorSets[SetIndex]->GetLayout();
	}

	vkCmdBindDescriptorSets(
		CommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		Pipeline.GetPipelineLayout(),
		0,
		VulkanDescriptorSets.size(),
		VulkanDescriptorSets.data(),
		0,
		nullptr);
}

void VulkanCommandList::BindVertexBuffers(uint32 NumVertexBuffers, const VulkanBuffer* VertexBuffers)
{
	std::vector<VkDeviceSize> Offsets(NumVertexBuffers);
	std::vector<VkBuffer> Buffers(NumVertexBuffers);

	for (uint32 Location = 0; Location < NumVertexBuffers; Location++)
	{
		Offsets[Location] = VertexBuffers[Location].GetOffset();
		Buffers[Location] = VertexBuffers[Location].GetVulkanHandle();
	}

	vkCmdBindVertexBuffers(CommandBuffer, 0, Buffers.size(), Buffers.data(), Offsets.data());
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
	vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.GetVulkanHandle(), IndexBuffer.GetOffset(), IndexType == EIndexType::UINT32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
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
		Buffer.GetVulkanHandle(),
		Buffer.GetOffset() + Offset,
		DrawCount,
		DrawCount > 1 ? sizeof(VkDrawIndirectCommand) : 0);
}

void VulkanCommandList::ClearColorImage(const VulkanImage& Image, EImageLayout ImageLayout, const ClearColorValue& Color)
{
	VkImageSubresourceRange Range = {};
	Range.aspectMask = Image.GetVulkanAspect();
	Range.baseArrayLayer = 0;
	Range.baseMipLevel = 0;
	Range.layerCount = 1;
	Range.levelCount = 1;

	vkCmdClearColorImage(CommandBuffer, Image.Image, VulkanImage::GetVulkanLayout(ImageLayout), reinterpret_cast<const VkClearColorValue*>(&Color), 1, &Range);
}

void VulkanCommandList::ClearDepthStencilImage(const VulkanImage& Image, EImageLayout ImageLayout, const ClearDepthStencilValue& DepthStencilValue)
{
	VkImageSubresourceRange Range = {};
	Range.aspectMask = Image.GetVulkanAspect();
	Range.baseArrayLayer = 0;
	Range.baseMipLevel = 0;
	Range.layerCount = 1;
	Range.levelCount = 1;

	vkCmdClearDepthStencilImage(CommandBuffer, Image.Image, VulkanImage::GetVulkanLayout(ImageLayout), reinterpret_cast<const VkClearDepthStencilValue*>(&DepthStencilValue), 1, &Range);
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
	uint32 NumBufferBarriers,
	const BufferMemoryBarrier* BufferBarriers,
	uint32 NumImageBarriers,
	const ImageMemoryBarrier* ImageBarriers)
{
	std::vector<VkBufferMemoryBarrier> VulkanBufferBarriers;
	VulkanBufferBarriers.reserve(NumBufferBarriers);

	for (uint32 BarrierIndex = 0; BarrierIndex < NumBufferBarriers; BarrierIndex++)
	{
		const BufferMemoryBarrier& BufferBarrier = BufferBarriers[BarrierIndex];
		const VulkanBuffer& Buffer = BufferBarrier.Buffer;

		VkBufferMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		Barrier.srcAccessMask = GetVulkanAccessFlags(BufferBarrier.SrcAccessMask);
		Barrier.dstAccessMask = GetVulkanAccessFlags(BufferBarrier.DstAccessMask);
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.buffer = Buffer.GetVulkanHandle();
		Barrier.offset = Buffer.GetOffset();
		Barrier.size = Buffer.GetSize();

		VulkanBufferBarriers.push_back(Barrier);
	}
	
	std::vector<VkImageMemoryBarrier> VulkanImageBarriers;
	VulkanImageBarriers.reserve(NumImageBarriers);

	for (uint32 BarrierIndex = 0; BarrierIndex < NumImageBarriers; BarrierIndex++)
	{
		const ImageMemoryBarrier& ImageBarrier = ImageBarriers[BarrierIndex];
		const drm::Image& Image = ImageBarrier.Image;

		VkImageMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		Barrier.srcAccessMask = GetVulkanAccessFlags(ImageBarrier.SrcAccessMask);
		Barrier.dstAccessMask = GetVulkanAccessFlags(ImageBarrier.DstAccessMask);
		Barrier.oldLayout = VulkanImage::GetVulkanLayout(ImageBarrier.OldLayout);
		Barrier.newLayout = VulkanImage::GetVulkanLayout(ImageBarrier.NewLayout);
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.image = Image.Image;
		Barrier.subresourceRange.aspectMask = Image.GetVulkanAspect();
		Barrier.subresourceRange.baseMipLevel = 0;
		Barrier.subresourceRange.levelCount = 1;
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
		VulkanBufferBarriers.size(), VulkanBufferBarriers.data(),
		VulkanImageBarriers.size(), VulkanImageBarriers.data()
	);
}

void VulkanCommandList::CopyBufferToImage(const VulkanBuffer& SrcBuffer, uint32 BufferOffset, const VulkanImage& DstImage, EImageLayout DstImageLayout)
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

	vkCmdCopyBufferToImage(CommandBuffer, SrcBuffer.GetVulkanHandle(), DstImage.Image, VulkanImage::GetVulkanLayout(DstImageLayout), Regions.size(), Regions.data());
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
		SrcImage.Image,
		VulkanImage::GetVulkanLayout(SrcImageLayout),
		DstImage.Image,
		VulkanImage::GetVulkanLayout(DstImageLayout),
		1,
		&Region,
		VulkanImage::GetVulkanFilter(Filter)
	);
}

void VulkanCommandList::SetScissor(uint32 ScissorCount, const ScissorDesc* Scissors)
{
	static_assert(sizeof(ScissorDesc) == sizeof(VkRect2D));
	vkCmdSetScissor(CommandBuffer, 0, ScissorCount, reinterpret_cast<const VkRect2D*>(Scissors));
}

void VulkanCommandList::CopyBuffer(const VulkanBuffer& SrcBuffer, const VulkanBuffer& DstBuffer, uint32 SrcOffset, uint32 DstOffset, uint32 Size)
{
	const VkBufferCopy Region = 
	{ 
		SrcBuffer.GetOffset() + SrcOffset, 
		DstBuffer.GetOffset() + DstOffset,
		Size 
	};

	vkCmdCopyBuffer(CommandBuffer, SrcBuffer.GetVulkanHandle(), DstBuffer.GetVulkanHandle(), 1, &Region);
}
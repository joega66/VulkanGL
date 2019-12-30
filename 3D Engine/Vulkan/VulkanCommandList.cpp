#include "VulkanCommandList.h"
#include "VulkanDRM.h"

VulkanCommandList::VulkanCommandList(VulkanDevice& Device, VulkanAllocator& Allocator, VkQueueFlagBits QueueFlags)
	: Device(Device)
	, Allocator(Allocator)
	, Queue(Device.Queues.GetQueue(QueueFlags))
	, CommandPool(Device.Queues.GetCommandPool(QueueFlags))
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

void VulkanCommandList::BeginRenderPass(const RenderPassInitializer& RPInit)
{
	for (uint32_t ColorTargetIndex = 0; ColorTargetIndex < RPInit.NumRenderTargets; ColorTargetIndex++)
	{
		// Determine if this command list touched the surface.
		if (RPInit.ColorTargets[ColorTargetIndex].Image == drm::GetSurface())
		{
			bTouchedSurface = true;
		}
	}

	VkFramebuffer Framebuffer;
	std::tie(Pending.RenderPass, Framebuffer) = Device.GetRenderPass(RPInit);

	const uint32 NumRTs = RPInit.NumRenderTargets;
	std::vector<VkClearValue> ClearValues;

	if (RPInit.DepthTarget.Image)
	{
		ClearValues.resize(NumRTs + 1);
	}
	else
	{
		ClearValues.resize(NumRTs);
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < NumRTs; ColorTargetIndex++)
	{
		const auto& ClearValue = std::get<ClearColorValue>(RPInit.ColorTargets[ColorTargetIndex].ClearValue);
		memcpy(ClearValues[ColorTargetIndex].color.float32, ClearValue.Float32, sizeof(ClearValue.Float32));
		memcpy(ClearValues[ColorTargetIndex].color.int32, ClearValue.Int32, sizeof(ClearValue.Int32));
		memcpy(ClearValues[ColorTargetIndex].color.uint32, ClearValue.Uint32, sizeof(ClearValue.Uint32));
	}

	if (RPInit.DepthTarget.Image)
	{
		VulkanImageRef Image = ResourceCast(RPInit.DepthTarget.Image);

		ClearValues[NumRTs].depthStencil = { 0, 0 };

		if (Image->IsDepth())
		{
			ClearValues[NumRTs].depthStencil.depth = std::get<ClearDepthStencilValue>(RPInit.DepthTarget.ClearValue).DepthClear;
		}

		if (Image->IsStencil())
		{
			ClearValues[NumRTs].depthStencil.stencil = std::get<ClearDepthStencilValue>(RPInit.DepthTarget.ClearValue).StencilClear;
		}
	}

	VkRect2D RenderArea;
	RenderArea.offset.x = RPInit.RenderArea.Offset.x;
	RenderArea.offset.y = RPInit.RenderArea.Offset.y;
	RenderArea.extent.width = RPInit.RenderArea.Extent.x;
	RenderArea.extent.height = RPInit.RenderArea.Extent.y;

	VkRenderPassBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	BeginInfo.renderPass = Pending.RenderPass;
	BeginInfo.framebuffer = Framebuffer;
	BeginInfo.renderArea = RenderArea;
	BeginInfo.pClearValues = ClearValues.data();
	BeginInfo.clearValueCount = ClearValues.size();

	vkCmdBeginRenderPass(CommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	Pending.NumRenderTargets = RPInit.NumRenderTargets;
}

void VulkanCommandList::EndRenderPass()
{
	vkCmdEndRenderPass(CommandBuffer);
}

void VulkanCommandList::BindPipeline(const PipelineStateInitializer& PSOInit)
{
	const GraphicsPipelineState& PipelineState = PSOInit.GraphicsPipelineState;
	// Validate gfx pipeline stages.
	assert(PipelineState.Vertex && PipelineState.Vertex->CompilationInfo.Stage == EShaderStage::Vertex);
	assert(!PipelineState.TessControl || (PipelineState.TessControl && PipelineState.TessControl->CompilationInfo.Stage == EShaderStage::TessControl));
	assert(!PipelineState.TessEval || (PipelineState.TessEval && PipelineState.TessEval->CompilationInfo.Stage == EShaderStage::TessEvaluation));
	assert(!PipelineState.Geometry || (PipelineState.Geometry && PipelineState.Geometry->CompilationInfo.Stage == EShaderStage::Geometry));
	assert(!PipelineState.Fragment || (PipelineState.Fragment && PipelineState.Fragment->CompilationInfo.Stage == EShaderStage::Fragment));

	VkPipeline Pipeline = Device.GetPipeline(PSOInit, Pending.PipelineLayout, Pending.RenderPass, Pending.NumRenderTargets);
	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
}

void VulkanCommandList::BindDescriptorSets(uint32 NumDescriptorSets, const drm::DescriptorSetRef* DescriptorSets)
{
	std::vector<VkDescriptorSet> VulkanDescriptorSets(NumDescriptorSets);
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts(NumDescriptorSets);

	for (uint32 SetIndex = 0; SetIndex < NumDescriptorSets; SetIndex++)
	{
		VulkanDescriptorSetRef VulkanDescriptorSet = ResourceCast(DescriptorSets[SetIndex]);
		VulkanDescriptorSets[SetIndex] = VulkanDescriptorSet->DescriptorSet;
		DescriptorSetLayouts[SetIndex] = VulkanDescriptorSet->DescriptorSetLayout;
	}

	Pending.PipelineLayout = Device.GetPipelineLayout(DescriptorSetLayouts);

	vkCmdBindDescriptorSets(
		CommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		Pending.PipelineLayout,
		0,
		VulkanDescriptorSets.size(),
		VulkanDescriptorSets.data(),
		0,
		nullptr);
}

void VulkanCommandList::BindVertexBuffers(uint32 NumVertexBuffers, const drm::BufferRef* VertexBuffers)
{
	check(NumVertexBuffers < Device.Properties.limits.maxVertexInputBindings, "maxVertexInputBindings exceeded.");

	std::vector<VkDeviceSize> Offsets(NumVertexBuffers);
	std::vector<VkBuffer> Buffers(NumVertexBuffers);

	for (uint32 Location = 0; Location < NumVertexBuffers; Location++)
	{
		check(VertexBuffers, "Null vertex buffer found.");
		VulkanBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffers[Location]);
		Offsets[Location] = VulkanVertexBuffer->Memory->Offset;
		Buffers[Location] = VulkanVertexBuffer->Memory->GetVulkanHandle();
	}

	vkCmdBindVertexBuffers(CommandBuffer, 0, Buffers.size(), Buffers.data(), Offsets.data());
}

void VulkanCommandList::DrawIndexed(drm::BufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance)
{
	VulkanBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	vkCmdBindIndexBuffer(CommandBuffer, VulkanIndexBuffer->Memory->GetVulkanHandle(), VulkanIndexBuffer->Memory->Offset, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(CommandBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void VulkanCommandList::Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
{
	vkCmdDraw(CommandBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

void VulkanCommandList::DrawIndirect(drm::BufferRef Buffer, uint32 Offset, uint32 DrawCount)
{
	const VulkanBufferRef& VulkanBuffer = ResourceCast(Buffer);
	vkCmdDrawIndirect(
		CommandBuffer, 
		VulkanBuffer->Memory->GetVulkanHandle(), 
		VulkanBuffer->Memory->Offset + Offset, 
		DrawCount, 
		DrawCount > 1 ? sizeof(VkDrawIndirectCommand) : 0);
}

void VulkanCommandList::Finish()
{
	vulkan(vkEndCommandBuffer(CommandBuffer));
	bFinished = true;
}

void VulkanCommandList::ClearColorImage(drm::ImageRef Image, const ClearColorValue& Color)
{
	const VulkanImageRef& VulkanImage = ResourceCast(Image);

	VkImageSubresourceRange Range = {};
	Range.aspectMask = VulkanImage->GetVulkanAspect();
	Range.baseArrayLayer = 0;
	Range.baseMipLevel = 0;
	Range.layerCount = 1;
	Range.levelCount = 1;

	vkCmdClearColorImage(CommandBuffer, VulkanImage->Image, VulkanImage->GetVulkanLayout(), reinterpret_cast<const VkClearColorValue*>(&Color), 1, &Range);
}

void VulkanCommandList::ClearDepthStencilImage(drm::ImageRef Image, const ClearDepthStencilValue& DepthStencilValue)
{
	const VulkanImageRef& VulkanImage = ResourceCast(Image);

	VkImageSubresourceRange Range = {};
	Range.aspectMask = VulkanImage->GetVulkanAspect();
	Range.baseArrayLayer = 0;
	Range.baseMipLevel = 0;
	Range.layerCount = 1;
	Range.levelCount = 1;

	vkCmdClearDepthStencilImage(CommandBuffer, VulkanImage->Image, VulkanImage->GetVulkanLayout(), reinterpret_cast<const VkClearDepthStencilValue*>(&DepthStencilValue), 1, &Range);
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
		const VulkanBufferRef& VulkanBuffer = ResourceCast(BufferBarrier.Buffer);

		VkBufferMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		Barrier.srcAccessMask = GetVulkanAccessFlags(BufferBarrier.SrcAccessMask);
		Barrier.dstAccessMask = GetVulkanAccessFlags(BufferBarrier.DstAccessMask);
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.buffer = VulkanBuffer->Memory->GetVulkanHandle();
		Barrier.offset = VulkanBuffer->Memory->Offset;
		Barrier.size = VulkanBuffer->Memory->Size;

		VulkanBufferBarriers.push_back(Barrier);
	}
	
	std::vector<VkImageMemoryBarrier> VulkanImageBarriers;
	VulkanImageBarriers.reserve(NumImageBarriers);

	for (uint32 BarrierIndex = 0; BarrierIndex < NumImageBarriers; BarrierIndex++)
	{
		const ImageMemoryBarrier& ImageBarrier = ImageBarriers[BarrierIndex];
		const VulkanImageRef& VulkanImage = ResourceCast(ImageBarrier.Image);

		VkImageMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		Barrier.srcAccessMask = GetVulkanAccessFlags(ImageBarrier.SrcAccessMask);
		Barrier.dstAccessMask = GetVulkanAccessFlags(ImageBarrier.DstAccessMask);
		Barrier.oldLayout = VulkanImage->GetVulkanLayout();
		Barrier.newLayout = VulkanImage::GetVulkanLayout(ImageBarrier.NewLayout);
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.image = VulkanImage->Image;
		Barrier.subresourceRange.aspectMask = VulkanImage->GetVulkanAspect();
		Barrier.subresourceRange.baseMipLevel = 0;
		Barrier.subresourceRange.levelCount = 1;
		Barrier.subresourceRange.baseArrayLayer = 0;
		Barrier.subresourceRange.layerCount = Any(VulkanImage->Usage & EImageUsage::Cubemap) ? 6 : 1;

		VulkanImageBarriers.push_back(Barrier);

		VulkanImage->Layout = ImageBarrier.NewLayout;
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
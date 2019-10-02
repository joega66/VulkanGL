#include "VulkanCommandList.h"
#include "VulkanDRM.h"

static CAST(drm::RenderTargetView, VulkanRenderTargetView);
static CAST(drm::Image, VulkanImage);
static CAST(drm::VertexBuffer, VulkanVertexBuffer);
static CAST(drm::UniformBuffer, VulkanUniformBuffer);
static CAST(drm::StorageBuffer, VulkanStorageBuffer);
static CAST(drm::IndexBuffer, VulkanIndexBuffer);
static CAST(RenderCommandList, VulkanCommandList);
static CAST(drm::DescriptorSet, VulkanDescriptorSet);

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
	// Determine if this command list touched the surface.
	for (uint32_t ColorTargetIndex = 0; ColorTargetIndex < RPInit.NumRenderTargets; ColorTargetIndex++)
	{
		if (RPInit.ColorTargets[ColorTargetIndex]->Image == drm::GetSurface())
		{
			bTouchedSurface = true;
		}
	}

	VkFramebuffer Framebuffer;
	std::tie(Pending.RenderPass, Framebuffer) = Device.GetRenderPass(RPInit);

	const uint32 NumRTs = RPInit.NumRenderTargets;
	std::vector<VkClearValue> ClearValues;

	if (RPInit.DepthTarget)
	{
		ClearValues.resize(NumRTs + 1);
	}
	else
	{
		ClearValues.resize(NumRTs);
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < NumRTs; ColorTargetIndex++)
	{
		const auto& ClearValue = std::get<ClearColorValue>(RPInit.ColorTargets[ColorTargetIndex]->ClearValue);
		memcpy(ClearValues[ColorTargetIndex].color.float32, ClearValue.Float32, sizeof(ClearValue.Float32));
		memcpy(ClearValues[ColorTargetIndex].color.int32, ClearValue.Int32, sizeof(ClearValue.Int32));
		memcpy(ClearValues[ColorTargetIndex].color.uint32, ClearValue.Uint32, sizeof(ClearValue.Uint32));
	}

	if (RPInit.DepthTarget)
	{
		VulkanRenderTargetViewRef DepthTarget = ResourceCast(RPInit.DepthTarget);
		VulkanImageRef Image = ResourceCast(DepthTarget->Image);

		ClearValues[NumRTs].depthStencil = { 0, 0 };

		if (Image->IsDepth())
		{
			ClearValues[NumRTs].depthStencil.depth = std::get<ClearDepthStencilValue>(RPInit.DepthTarget->ClearValue).DepthClear;
		}

		if (Image->IsStencil())
		{
			ClearValues[NumRTs].depthStencil.stencil = std::get<ClearDepthStencilValue>(RPInit.DepthTarget->ClearValue).StencilClear;
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
	VkPipeline Pipeline = Device.GetPipeline(PSOInit, Pending.PipelineLayout, Pending.RenderPass, Pending.NumRenderTargets);
	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
}

void VulkanCommandList::BindDescriptorSets(uint32 NumDescriptorSets, const drm::DescriptorSetRef* DescriptorSets)
{
	std::vector<VkDescriptorSet> VulkanDescriptorSets;
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;

	for (uint32 SetIndex = 0; SetIndex < NumDescriptorSets; SetIndex++)
	{
		VulkanDescriptorSetRef VulkanDescriptorSet = ResourceCast(DescriptorSets[SetIndex]);
		VulkanDescriptorSets.push_back(VulkanDescriptorSet->DescriptorSet);
		DescriptorSetLayouts.push_back(VulkanDescriptorSet->DescriptorSetLayout);
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

void VulkanCommandList::BindVertexBuffers(uint32 NumVertexBuffers, const drm::VertexBufferRef* VertexBuffers)
{
	check(NumVertexBuffers < Device.Properties.limits.maxVertexInputBindings, "maxVertexInputBindings exceeded.");

	std::vector<VkDeviceSize> Offsets(NumVertexBuffers);
	std::vector<VkBuffer> Buffers(NumVertexBuffers);

	for (uint32 Location = 0; Location < NumVertexBuffers; Location++)
	{
		check(VertexBuffers, "Null vertex buffer found.");
		VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffers[Location]);
		Offsets[Location] = VulkanVertexBuffer->Buffer->Offset;
		Buffers[Location] = VulkanVertexBuffer->Buffer->GetVulkanHandle();
	}

	vkCmdBindVertexBuffers(CommandBuffer, 0, Buffers.size(), Buffers.data(), Offsets.data());
}

void VulkanCommandList::DrawIndexed(drm::IndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance)
{
	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	const VkIndexType IndexType = VulkanIndexBuffer->IndexStride == sizeof(uint32) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

	vkCmdBindIndexBuffer(CommandBuffer, VulkanIndexBuffer->Buffer->GetVulkanHandle(), VulkanIndexBuffer->Buffer->Offset, IndexType);
	vkCmdDrawIndexed(CommandBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void VulkanCommandList::Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
{
	vkCmdDraw(CommandBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

void VulkanCommandList::Finish()
{
	vulkan(vkEndCommandBuffer(CommandBuffer));
	bFinished = true;
}
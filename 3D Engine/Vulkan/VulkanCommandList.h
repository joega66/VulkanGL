#pragma once
#include "../DRMCommandList.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanDescriptors.h"

class VulkanQueues;

class VulkanCommandList final : public drm::CommandList
{
public:
	VkQueue Queue;

	VkCommandPool CommandPool;

	VkCommandBuffer CommandBuffer;

	VulkanCommandList(VulkanDevice& Device, VulkanQueues& Queues, VkQueueFlagBits QueueFlags);

	virtual ~VulkanCommandList() override;

	virtual void BeginRenderPass(drm::RenderPassRef RenderPass) override;

	virtual void BeginRenderPass(const RenderPassInitializer& RenderPassInit) override;

	virtual void EndRenderPass() override;

	virtual void BindPipeline(const PipelineStateInitializer& PSOInit) override;

	virtual void BindDescriptorSets(uint32 NumDescriptorSets, const drm::DescriptorSetRef* DescriptorSets) override;

	virtual void BindVertexBuffers(uint32 NumVertexBuffers, const drm::BufferRef* VertexBuffers) override;

	virtual void DrawIndexed(drm::BufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance) override;

	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance) override;

	virtual void DrawIndirect(drm::BufferRef Buffer, uint32 Offset, uint32 DrawCount) override;

	virtual void ClearColorImage(drm::ImageRef Image, const ClearColorValue& Color) override;

	virtual void ClearDepthStencilImage(drm::ImageRef Image, const ClearDepthStencilValue& DepthStencilValue) override;

	virtual void PipelineBarrier(
		EPipelineStage SrcStageMask,
		EPipelineStage DstStageMask,
		uint32 NumBufferBarriers,
		const BufferMemoryBarrier* BufferBarriers,
		uint32 NumImageBarriers,
		const ImageMemoryBarrier* ImageBarriers
	) override;

	virtual void CopyBufferToImage(drm::BufferRef SrcBuffer, uint32 BufferOffset, drm::ImageRef DstImage) override;

	virtual void BlitImage(drm::ImageRef SrcImage, drm::ImageRef DstImage, EFilter Filter) override;

private:
	VulkanDevice& Device;

	struct PendingState
	{
		uint32 NumRenderTargets;
		VkRenderPass RenderPass;
		VkPipelineLayout PipelineLayout;
	} Pending;
};

CLASS(VulkanCommandList);
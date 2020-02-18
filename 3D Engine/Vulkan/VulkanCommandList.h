#pragma once
#include "VulkanMemory.h"
#include "VulkanDescriptors.h"

class VulkanDevice;
class VulkanImage;
struct ImageMemoryBarrier;
struct BufferMemoryBarrier;

class VulkanCommandList
{
public:
	VkQueue Queue;

	VkCommandPool CommandPool;

	VkCommandBuffer CommandBuffer;

	/** @begin drm::CommandList */

	VulkanCommandList(VulkanDevice& Device, VkQueueFlagBits QueueFlags);
	~VulkanCommandList();
	VulkanCommandList(const VulkanCommandList&) = delete;
	VulkanCommandList& operator=(const VulkanCommandList&) = delete;

	void BeginRenderPass(const class VulkanRenderPass& RenderPass);

	void EndRenderPass();

	void BindPipeline(const struct PipelineStateDesc& PSODesc);

	void BindDescriptorSets(uint32 NumDescriptorSets, const drm::DescriptorSetRef* DescriptorSets);

	void BindVertexBuffers(uint32 NumVertexBuffers, const drm::BufferRef* VertexBuffers);

	void DrawIndexed(
		drm::BufferRef IndexBuffer, 
		uint32 IndexCount, 
		uint32 InstanceCount, 
		uint32 FirstIndex, 
		uint32 VertexOffset, 
		uint32 FirstInstance,
		EIndexType IndexType = EIndexType::UINT32
	);

	void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance);

	void DrawIndirect(drm::BufferRef Buffer, uint32 Offset, uint32 DrawCount);

	void ClearColorImage(const VulkanImage& Image, EImageLayout ImageLayout, const ClearColorValue& Color);

	void ClearDepthStencilImage(const VulkanImage& Image, EImageLayout ImageLayout, const ClearDepthStencilValue& DepthStencilValue);

	void PipelineBarrier(
		EPipelineStage SrcStageMask,
		EPipelineStage DstStageMask,
		uint32 NumBufferBarriers,
		const BufferMemoryBarrier* BufferBarriers,
		uint32 NumImageBarriers,
		const ImageMemoryBarrier* ImageBarriers
	);

	void CopyBufferToImage(
		drm::BufferRef SrcBuffer, 
		uint32 BufferOffset, 
		const VulkanImage& DstImage,
		EImageLayout DstImageLayout
	);

	void BlitImage(
		const VulkanImage& SrcImage,
		EImageLayout SrcImageLayout,
		const VulkanImage& DstImage,
		EImageLayout DstImageLayout,
		EFilter Filter
	);

	void SetScissor(uint32 ScissorCount, const ScissorDesc* Scissors);

	void CopyBuffer(
		drm::BufferRef SrcBuffer,
		drm::BufferRef DstBuffer,
		uint32 SrcOffset,
		uint32 DstOffset,
		uint32 Size
	);

	/** @end */

private:
	VulkanDevice& Device;

	struct PendingState
	{
		VkPipelineLayout PipelineLayout;
	} Pending;
};

CLASS(VulkanCommandList);
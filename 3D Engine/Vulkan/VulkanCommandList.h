#pragma once
#include <DRMResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;
class VulkanBuffer;
class VulkanImage;
struct ImageMemoryBarrier;
struct BufferMemoryBarrier;
class VulkanPipeline;

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

	void BindPipeline(const std::shared_ptr<VulkanPipeline>& Pipeline);

	void BindDescriptorSets(const std::shared_ptr<VulkanPipeline>& Pipeline, uint32 NumDescriptorSets, const VkDescriptorSet* DescriptorSets);

	void BindVertexBuffers(uint32 NumVertexBuffers, const VulkanBuffer* VertexBuffers);

	void DrawIndexed(
		const VulkanBuffer& IndexBuffer,
		uint32 IndexCount, 
		uint32 InstanceCount, 
		uint32 FirstIndex, 
		uint32 VertexOffset, 
		uint32 FirstInstance,
		EIndexType IndexType = EIndexType::UINT32
	);

	void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance);

	void DrawIndirect(const VulkanBuffer& Buffer, uint32 Offset, uint32 DrawCount);

	void Dispatch(uint32 GroupCountX, uint32 GroupCountY, uint32 GroupCountZ);

	void ClearColorImage(const VulkanImage& Image, EImageLayout ImageLayout, const ClearColorValue& Color);

	void ClearDepthStencilImage(const VulkanImage& Image, EImageLayout ImageLayout, const ClearDepthStencilValue& DepthStencilValue);

	void PipelineBarrier(
		EPipelineStage SrcStageMask,
		EPipelineStage DstStageMask,
		std::size_t NumBufferBarriers,
		const BufferMemoryBarrier* BufferBarriers,
		std::size_t NumImageBarriers,
		const ImageMemoryBarrier* ImageBarriers
	);

	void CopyBufferToImage(
		const VulkanBuffer& SrcBuffer, 
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
		const VulkanBuffer& SrcBuffer,
		const VulkanBuffer& DstBuffer,
		uint64 SrcOffset,
		uint64 DstOffset,
		uint64 Size
	);

	/** @end */

private:
	VulkanDevice& Device;
};
#pragma once
#include <GPU/GPUShader.h>
#include <vulkan/vulkan.h>

class VulkanDevice;
class VulkanBuffer;
class VulkanImage;
struct ImageMemoryBarrier;
struct BufferMemoryBarrier;
class VulkanPipeline;

class VulkanCommandList
{
	VulkanCommandList(const VulkanCommandList&) = delete;
	VulkanCommandList& operator=(const VulkanCommandList&) = delete;
public:
	VkQueue _Queue;

	VkCommandPool _CommandPool;

	VkCommandBuffer _CommandBuffer;

	VulkanCommandList(VulkanDevice& device, VkQueueFlags queueFlags);

	~VulkanCommandList();

	void BeginRenderPass(const class VulkanRenderPass& renderPass);

	void EndRenderPass();

	void BindPipeline(const std::shared_ptr<VulkanPipeline>& pipeline);

	void BindDescriptorSets(const std::shared_ptr<VulkanPipeline>& pipeline, uint32 numDescriptorSets, const VkDescriptorSet* descriptorSets);

	void PushConstants(const std::shared_ptr<VulkanPipeline>& pipeline, EShaderStage stageFlags, uint32 offset, uint32 size, const void* values);

	void PushConstants(const std::shared_ptr<VulkanPipeline>& pipeline, const gpu::Shader* shader, const void* values);

	void BindVertexBuffers(uint32 numVertexBuffers, const VulkanBuffer* vertexBuffers);

	void DrawIndexed(
		const VulkanBuffer& indexBuffer,
		uint32 indexCount, 
		uint32 instanceCount, 
		uint32 firstIndex, 
		uint32 vertexOffset, 
		uint32 firstInstance,
		EIndexType indexType
	);

	void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance);

	void DrawIndirect(const VulkanBuffer& buffer, uint32 offset, uint32 drawCount);

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

	void SetScissor(uint32 ScissorCount, const Scissor* Scissors);

	void CopyBuffer(
		const VulkanBuffer& SrcBuffer,
		const VulkanBuffer& DstBuffer,
		uint64 SrcOffset,
		uint64 DstOffset,
		uint64 Size
	);

	void CopyImage(
		const VulkanImage& SrcImage,
		EImageLayout SrcImageLayout,
		const VulkanImage& DstImage,
		EImageLayout DstImageLayout,
		uint32 DstArrayLayer
	);

private:
	VulkanDevice& _Device;
};
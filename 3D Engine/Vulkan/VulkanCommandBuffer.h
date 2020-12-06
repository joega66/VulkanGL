#pragma once
#include <GPU/GPUShader.h>
#include <vulkan/vulkan.h>

class VulkanDevice;
struct ImageMemoryBarrier;
struct BufferMemoryBarrier;
class VulkanQueue;

namespace gpu
{
	class Buffer;
	class Image;
	class RenderPass;
	class Pipeline;

	class CommandBuffer
	{
		CommandBuffer(const CommandBuffer&) = delete;
		CommandBuffer& operator=(const CommandBuffer&) = delete;
	public:
		VulkanQueue& _Queue;

		VkCommandBuffer _CommandBuffer;

		CommandBuffer(VulkanDevice& device, VulkanQueue& queue);

		void BeginRenderPass(const RenderPass& renderPass);

		void EndRenderPass();

		void BindPipeline(const gpu::Pipeline& pipeline);

		void BindDescriptorSets(
			const gpu::Pipeline& pipeline,
			std::size_t numDescriptorSets,
			const VkDescriptorSet* descriptorSets,
			std::size_t numDynamicOffsets,
			const uint32* dynamicOffsets
		);

		void PushConstants(
			const gpu::Pipeline& pipeline,
			const Shader* shader, 
			const void* values
		);

		void BindVertexBuffers(
			std::size_t numVertexBuffers, 
			const Buffer* vertexBuffers
		);

		void DrawIndexed(
			const Buffer& indexBuffer,
			uint32 indexCount,
			uint32 instanceCount,
			uint32 firstIndex,
			uint32 vertexOffset,
			uint32 firstInstance,
			EIndexType indexType
		);

		void Draw(
			uint32 vertexCount, 
			uint32 instanceCount, 
			uint32 firstVertex, 
			uint32 firstInstance
		);

		void DrawIndirect(
			const Buffer& buffer, 
			uint32 offset, 
			uint32 drawCount
		);

		void Dispatch(
			uint32 groupCountX, 
			uint32 groupCountY, 
			uint32 groupCountZ
		);

		void ClearColorImage(
			const Image& image, 
			EImageLayout imageLayout, 
			const ClearColorValue& color
		);

		void ClearDepthStencilImage(
			const Image& image, 
			EImageLayout imageLayout, 
			const ClearDepthStencilValue& depthStencilValue
		);

		void PipelineBarrier(
			EPipelineStage srcStageMask,
			EPipelineStage dstStageMask,
			std::size_t numBufferBarriers,
			const BufferMemoryBarrier* bufferBarriers,
			std::size_t numImageBarriers,
			const ImageMemoryBarrier* imageBarriers
		);

		void CopyBufferToImage(
			const Buffer& srcBuffer,
			uint32 bufferOffset,
			const Image& dstImage,
			EImageLayout dstImageLayout
		);

		void BlitImage(
			const Image& srcImage,
			EImageLayout srcImageLayout,
			const Image& dstImage,
			EImageLayout dstImageLayout,
			EFilter filter
		);

		void CopyBuffer(
			const Buffer& srcBuffer,
			const Buffer& dstBuffer,
			uint64 srcOffset,
			uint64 dstOffset,
			uint64 size
		);

		void CopyImage(
			const Image& srcImage,
			EImageLayout srcImageLayout,
			const Image& dstImage,
			EImageLayout dstImageLayout,
			uint32 dstArrayLayer
		);

		std::shared_ptr<Buffer> CreateStagingBuffer(
			uint64 size, 
			const void* data = nullptr
		);

		void SetViewport(const Viewport& viewport);

		void SetScissor(const Scissor& scissor);

		void SetViewportAndScissor(const Viewport& viewport);

	private:
		VulkanDevice& _Device;
	};
};
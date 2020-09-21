#pragma once
#include <GPU/GPUShader.h>
#include <vulkan/vulkan.h>

class VulkanDevice;
struct ImageMemoryBarrier;
struct BufferMemoryBarrier;
class VulkanPipeline;

namespace gpu
{
	class Buffer;
	class Image;
	class RenderPass;

	class CommandList
	{
		CommandList(const CommandList&) = delete;
		CommandList& operator=(const CommandList&) = delete;
	public:
		VkQueue _Queue;

		VkCommandPool _CommandPool;

		VkCommandBuffer _CommandBuffer;

		CommandList(VulkanDevice& device, VkQueueFlags queueFlags);

		~CommandList();

		void BeginRenderPass(const RenderPass& renderPass);

		void EndRenderPass();

		void BindPipeline(const std::shared_ptr<VulkanPipeline>& pipeline);

		void BindDescriptorSets(const std::shared_ptr<VulkanPipeline>& pipeline, std::size_t numDescriptorSets, const VkDescriptorSet* descriptorSets);

		void PushConstants(const std::shared_ptr<VulkanPipeline>& pipeline, const gpu::Shader* shader, const void* values);

		void BindVertexBuffers(uint32 numVertexBuffers, const Buffer* vertexBuffers);

		void DrawIndexed(
			const Buffer& indexBuffer,
			uint32 indexCount,
			uint32 instanceCount,
			uint32 firstIndex,
			uint32 vertexOffset,
			uint32 firstInstance,
			EIndexType indexType
		);

		void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance);

		void DrawIndirect(const Buffer& buffer, uint32 offset, uint32 drawCount);

		void Dispatch(uint32 GroupCountX, uint32 GroupCountY, uint32 GroupCountZ);

		void ClearColorImage(const Image& Image, EImageLayout ImageLayout, const ClearColorValue& Color);

		void ClearDepthStencilImage(const Image& Image, EImageLayout ImageLayout, const ClearDepthStencilValue& DepthStencilValue);

		void PipelineBarrier(
			EPipelineStage SrcStageMask,
			EPipelineStage DstStageMask,
			std::size_t NumBufferBarriers,
			const BufferMemoryBarrier* BufferBarriers,
			std::size_t NumImageBarriers,
			const ImageMemoryBarrier* ImageBarriers
		);

		void CopyBufferToImage(
			const Buffer& SrcBuffer,
			uint32 BufferOffset,
			const Image& DstImage,
			EImageLayout DstImageLayout
		);

		void BlitImage(
			const Image& SrcImage,
			EImageLayout SrcImageLayout,
			const Image& DstImage,
			EImageLayout DstImageLayout,
			EFilter Filter
		);

		void SetScissor(uint32 ScissorCount, const Scissor* Scissors);

		void CopyBuffer(
			const Buffer& SrcBuffer,
			const Buffer& DstBuffer,
			uint64 SrcOffset,
			uint64 DstOffset,
			uint64 Size
		);

		void CopyImage(
			const Image& SrcImage,
			EImageLayout SrcImageLayout,
			const Image& DstImage,
			EImageLayout DstImageLayout,
			uint32 DstArrayLayer
		);

	private:
		VulkanDevice& _Device;
	};
};
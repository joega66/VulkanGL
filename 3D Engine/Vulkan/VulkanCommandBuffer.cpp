#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include <GPU/GPUShader.h>
#include <GPU/GPUDefinitions.h>

namespace gpu
{
	CommandBuffer::CommandBuffer(VulkanDevice& device, VulkanQueue& queue)
		: _Device(device)
		, _Queue(queue)
	{
		const VkCommandBufferAllocateInfo commandBufferAllocateInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = queue.GetCommandPool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		
		vulkan(vkAllocateCommandBuffers(_Device, &commandBufferAllocateInfo, &_CommandBuffer));

		const VkCommandBufferBeginInfo commandBufferBeginInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		vulkan(vkBeginCommandBuffer(_CommandBuffer, &commandBufferBeginInfo));
	}

	void CommandBuffer::BeginRenderPass(const RenderPass& renderPass)
	{
		const VkRenderPassBeginInfo renderPassBeginInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = renderPass.GetRenderPass(),
			.framebuffer = renderPass.GetFramebuffer(),
			.renderArea = renderPass.GetRenderArea(),
			.clearValueCount = static_cast<uint32>(renderPass.GetClearValues().size()),
			.pClearValues = renderPass.GetClearValues().data(),
		};
		
		vkCmdBeginRenderPass(
			_CommandBuffer, 
			&renderPassBeginInfo, 
			VK_SUBPASS_CONTENTS_INLINE
		);
	}

	void CommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(_CommandBuffer);
	}

	void CommandBuffer::BindPipeline(const gpu::Pipeline& pipeline)
	{
		vkCmdBindPipeline(
			_CommandBuffer, 
			pipeline.GetPipelineBindPoint(), 
			pipeline.GetPipeline()
		);
	}

	void CommandBuffer::BindDescriptorSets(
		const gpu::Pipeline& pipeline,
		std::size_t numDescriptorSets, 
		const VkDescriptorSet* descriptorSets,
		std::size_t numDynamicOffsets,
		const uint32* dynamicOffsets)
	{
		vkCmdBindDescriptorSets(
			_CommandBuffer,
			pipeline.GetPipelineBindPoint(),
			pipeline.GetPipelineLayout(),
			0,
			static_cast<uint32>(numDescriptorSets),
			descriptorSets,
			static_cast<uint32>(numDynamicOffsets),
			dynamicOffsets
		);
	}

	void CommandBuffer::PushConstants(const gpu::Pipeline& pipeline, const gpu::Shader* shader, const void* values)
	{
		const auto& pushConstantRange = shader->compilationResult.pushConstantRange;

		vkCmdPushConstants(
			_CommandBuffer,
			pipeline.GetPipelineLayout(),
			pushConstantRange.stageFlags,
			pushConstantRange.offset,
			pushConstantRange.size,
			values
		);
	}

	void CommandBuffer::BindVertexBuffers(std::size_t numVertexBuffers, const Buffer* vertexBuffers)
	{
		std::vector<VkDeviceSize> offsets(numVertexBuffers, 0);
		std::vector<VkBuffer> buffers(numVertexBuffers);

		for (uint32 location = 0; location < numVertexBuffers; location++)
		{
			buffers[location] = vertexBuffers[location];
		}

		vkCmdBindVertexBuffers(_CommandBuffer, 0, static_cast<uint32>(buffers.size()), buffers.data(), offsets.data());
	}

	void CommandBuffer::DrawIndexed(
		const Buffer& indexBuffer,
		uint32 indexCount,
		uint32 instanceCount,
		uint32 firstIndex,
		uint32 vertexOffset,
		uint32 firstInstance,
		EIndexType indexType)
	{
		vkCmdBindIndexBuffer(_CommandBuffer, indexBuffer, 0, static_cast<VkIndexType>(indexType));
		vkCmdDrawIndexed(_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandBuffer::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
		vkCmdDraw(_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandBuffer::DrawIndirect(const Buffer& buffer, uint32 offset, uint32 drawCount)
	{
		vkCmdDrawIndirect(
			_CommandBuffer,
			buffer,
			offset,
			drawCount,
			drawCount > 1 ? sizeof(VkDrawIndirectCommand) : 0
		);
	}

	void CommandBuffer::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		vkCmdDispatch(_CommandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void CommandBuffer::ClearColorImage(const Image& image, EImageLayout imageLayout, const ClearColorValue& color)
	{
		const VkImageSubresourceRange range = 
		{
			.aspectMask = image.GetVulkanAspect(),
			.baseMipLevel = 0,
			.levelCount = image.GetMipLevels(),
			.baseArrayLayer = 0,
			.layerCount = 1,
		};
		
		vkCmdClearColorImage(_CommandBuffer, image, Image::GetLayout(imageLayout), reinterpret_cast<const VkClearColorValue*>(&color), 1, &range);
	}

	void CommandBuffer::ClearDepthStencilImage(const Image& image, EImageLayout imageLayout, const ClearDepthStencilValue& depthStencilValue)
	{
		const VkImageSubresourceRange range = 
		{
			.aspectMask = image.GetVulkanAspect(),
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};
		
		vkCmdClearDepthStencilImage(_CommandBuffer, image, Image::GetLayout(imageLayout), reinterpret_cast<const VkClearDepthStencilValue*>(&depthStencilValue), 1, &range);
	}

	void CommandBuffer::PipelineBarrier(
		EPipelineStage srcStageMask,
		EPipelineStage dstStageMask,
		std::size_t numBufferBarriers,
		const BufferMemoryBarrier* bufferBarriers,
		std::size_t numImageBarriers,
		const ImageMemoryBarrier* imageBarriers)
	{
		std::vector<VkBufferMemoryBarrier> vulkanBufferBarriers;
		vulkanBufferBarriers.reserve(numBufferBarriers);

		for (std::size_t i = 0; i < numBufferBarriers; i++)
		{
			const VkBufferMemoryBarrier bufferBarrier = 
			{ 
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.srcAccessMask = VulkanDevice::GetAccessFlags(bufferBarriers[i].srcAccessMask),
				.dstAccessMask = VulkanDevice::GetAccessFlags(bufferBarriers[i].dstAccessMask),
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = bufferBarriers[i].buffer,
				.offset = 0,
				.size = bufferBarriers[i].buffer.GetSize(),
			};
			
			vulkanBufferBarriers.push_back(bufferBarrier);
		}

		std::vector<VkImageMemoryBarrier> vulkanImageBarriers;
		vulkanImageBarriers.reserve(numImageBarriers);

		for (std::size_t i = 0; i < numImageBarriers; i++)
		{
			const VkImageMemoryBarrier imageBarrier = 
			{ 
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VulkanDevice::GetAccessFlags(imageBarriers[i].srcAccessMask),
				.dstAccessMask = VulkanDevice::GetAccessFlags(imageBarriers[i].dstAccessMask),
				.oldLayout = Image::GetLayout(imageBarriers[i].oldLayout),
				.newLayout = Image::GetLayout(imageBarriers[i].newLayout),
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = imageBarriers[i].image,
				.subresourceRange = 
				{
					.aspectMask = imageBarriers[i].image.GetVulkanAspect(),
					.baseMipLevel = imageBarriers[i].baseMipLevel,
					.levelCount = imageBarriers[i].levelCount,
					.baseArrayLayer = 0,
					.layerCount = Any(imageBarriers[i].image.GetUsage() & EImageUsage::Cubemap) ? 6u : 1u,
				}
			};
			
			vulkanImageBarriers.push_back(imageBarrier);
		}

		vkCmdPipelineBarrier(
			_CommandBuffer,
			VulkanDevice::GetPipelineStageFlags(srcStageMask),
			VulkanDevice::GetPipelineStageFlags(dstStageMask),
			0,
			0, nullptr,
			static_cast<uint32>(vulkanBufferBarriers.size()), vulkanBufferBarriers.data(),
			static_cast<uint32>(vulkanImageBarriers.size()), vulkanImageBarriers.data()
		);
	}

	void CommandBuffer::CopyBufferToImage(
		const Buffer& srcBuffer,
		uint32 bufferOffset,
		const Image& dstImage,
		EImageLayout dstImageLayout
	)
	{
		std::vector<VkBufferImageCopy> regions;

		if (Any(dstImage.GetUsage() & EImageUsage::Cubemap))
		{
			constexpr uint32 numFaces = 6;

			regions.reserve(numFaces);

			const uint32 faceSize = dstImage.GetSize() / numFaces;

			for (uint32 i = 0; i < numFaces; i++)
			{
				// VkImageSubresourceRange(3) Manual Page:
				// "...the layers of the image view starting at baseArrayLayer correspond to faces in the order +X, -X, +Y, -Y, +Z, -Z"
				regions.push_back({
					.bufferOffset = i * faceSize,
					.bufferRowLength = 0,
					.bufferImageHeight = 0,
					.imageSubresource =
					{
						.aspectMask = dstImage.GetVulkanAspect(),
						.mipLevel = 0,
						.baseArrayLayer = i,
						.layerCount = 1,
					},
					.imageOffset = { 0, 0, 0 },
					.imageExtent = 
					{
						.width = dstImage.GetWidth(),
						.height = dstImage.GetHeight(),
						.depth = dstImage.GetDepth()
					}
				});
			}
		}
		else
		{
			regions.push_back({ 
				.bufferOffset = bufferOffset,
				.bufferRowLength = 0,
				.bufferImageHeight = 0,
				.imageSubresource =
				{
					.aspectMask = dstImage.GetVulkanAspect(),
					.mipLevel = 0,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.imageOffset = { 0, 0, 0 },
				.imageExtent =
				{
					.width = dstImage.GetWidth(),
					.height = dstImage.GetHeight(),
					.depth = dstImage.GetDepth()
				}
			});
		}

		vkCmdCopyBufferToImage(
			_CommandBuffer, 
			srcBuffer, 
			dstImage, 
			Image::GetLayout(dstImageLayout), 
			static_cast<uint32>(regions.size()), regions.data()
		);
	}

	void CommandBuffer::BlitImage(
		const Image& srcImage,
		EImageLayout srcImageLayout,
		const Image& dstImage,
		EImageLayout dstImageLayout,
		EFilter filter)
	{
		VkImageBlit region = {};

		region.srcSubresource.aspectMask = srcImage.GetVulkanAspect();
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;

		region.srcOffsets[1].x = srcImage.GetWidth();
		region.srcOffsets[1].y = srcImage.GetHeight();
		region.srcOffsets[1].z = 1;

		region.dstSubresource.aspectMask = dstImage.GetVulkanAspect();
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.layerCount = 1;

		region.dstOffsets[1].x = dstImage.GetWidth();
		region.dstOffsets[1].y = dstImage.GetHeight();
		region.dstOffsets[1].z = 1;

		vkCmdBlitImage(
			_CommandBuffer,
			srcImage,
			Image::GetLayout(srcImageLayout),
			dstImage,
			Image::GetLayout(dstImageLayout),
			1,
			&region,
			Image::GetVulkanFilter(filter)
		);
	}

	void CommandBuffer::SetViewport(const Viewport& viewport)
	{
		const VkViewport setViewport =
		{
			.x = static_cast<float>(viewport.x),
			.y = static_cast<float>(viewport.y),
			.width = static_cast<float>(viewport.width),
			.height = static_cast<float>(viewport.height),
			.minDepth = viewport.minDepth,
			.maxDepth = viewport.maxDepth,
		};

		vkCmdSetViewport(_CommandBuffer, 0, 1, &setViewport);
	}

	void CommandBuffer::SetScissor(const Scissor& scissor)
	{
		static_assert(sizeof(Scissor) == sizeof(VkRect2D));
		vkCmdSetScissor(_CommandBuffer, 0, 1, reinterpret_cast<const VkRect2D*>(&scissor));
	}

	void CommandBuffer::SetViewportAndScissor(const Viewport& viewport)
	{
		SetViewport(viewport);
		SetScissor({ .offset = { 0, 0 }, .extent = { viewport.width, viewport.height } });
	}

	void CommandBuffer::CopyBuffer(
		const Buffer& srcBuffer,
		const Buffer& dstBuffer,
		uint64 srcOffset,
		uint64 dstOffset,
		uint64 size)
	{
		const VkBufferCopy region =
		{
			srcOffset,
			dstOffset,
			size
		};

		vkCmdCopyBuffer(_CommandBuffer, srcBuffer, dstBuffer, 1, &region);
	}

	void CommandBuffer::CopyImage(
		const Image& srcImage,
		EImageLayout srcImageLayout,
		const Image& dstImage,
		EImageLayout dstImageLayout,
		uint32 dstArrayLayer)
	{
		VkImageCopy region = {};

		region.srcSubresource.aspectMask = srcImage.GetVulkanAspect();
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;

		region.dstSubresource.aspectMask = dstImage.GetVulkanAspect();
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.baseArrayLayer = dstArrayLayer;
		region.dstSubresource.layerCount = 1;

		region.extent.width = srcImage.GetWidth();
		region.extent.height = srcImage.GetHeight();
		region.extent.depth = 1;

		vkCmdCopyImage(
			_CommandBuffer,
			srcImage,
			Image::GetLayout(srcImageLayout),
			dstImage,
			Image::GetLayout(dstImageLayout),
			1,
			&region
		);
	}

	std::shared_ptr<gpu::Buffer> CommandBuffer::CreateStagingBuffer(uint64 size, const void* data)
	{
		auto stagingBuffer = std::make_shared<gpu::Buffer>(_Device.CreateBuffer(EBufferUsage::None, EMemoryUsage::CPU_ONLY, size, data));
		_Queue.AddInFlightStagingBuffer(stagingBuffer);
		return stagingBuffer;
	}
};
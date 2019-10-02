#pragma once
#include "../RenderCommandList.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanMemory.h"
#include "VulkanDescriptors.h"
#include "VulkanShader.h"

class VulkanCommandList final : public RenderCommandList
{
public:
	VkQueue Queue;

	VkCommandPool CommandPool;

	VkCommandBuffer CommandBuffer;

	bool bTouchedSurface = false;

	bool bFinished = false;

	VulkanCommandList(VulkanDevice& Device, VulkanAllocator& Allocator, VkQueueFlagBits QueueFlags);

	~VulkanCommandList();

	virtual void BeginRenderPass(const RenderPassInitializer& RenderPassInit);
	virtual void EndRenderPass();
	virtual void BindPipeline(const PipelineStateInitializer& PSOInit);
	virtual void BindDescriptorSets(uint32 NumDescriptorSets, const drm::DescriptorSetRef* DescriptorSets);
	virtual void BindVertexBuffers(uint32 NumVertexBuffers, const drm::VertexBufferRef* VertexBuffers);
	virtual void DrawIndexed(drm::IndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance);
	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance);
	virtual void Finish();

private:
	VulkanDevice& Device;

	VulkanAllocator& Allocator;

	struct PendingState
	{
		uint32 NumRenderTargets;
		VkRenderPass RenderPass;
		VkPipelineLayout PipelineLayout;
	} Pending;
};

CLASS(VulkanCommandList);
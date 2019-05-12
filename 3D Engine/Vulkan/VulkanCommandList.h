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
	const VkCommandBuffer CommandBuffer;
	bool bTouchedSurface = false;
	bool bFinished = false;

	VulkanCommandList(VulkanDevice& Device, VulkanAllocator& Allocator, VulkanDescriptorPool& DescriptorPool);
	~VulkanCommandList();

	virtual void SetRenderTargets(const RenderPassInitializer& RenderPassInit);
	virtual void SetPipelineState(const PipelineStateInitializer& PSOInit);
	virtual void SetVertexStream(uint32 Location, drm::VertexBufferRef VertexBuffer);
	virtual void SetUniformBuffer(drm::ShaderRef Shader, uint32 Location, drm::UniformBufferRef UniformBuffer);
	virtual void SetShaderImage(drm::ShaderRef Shader, uint32 Location, drm::ImageRef Image, const SamplerState& Sampler);
	virtual void SetStorageBuffer(drm::ShaderRef Shader, uint32 Location, drm::StorageBufferRef StorageBuffer);
	virtual void DrawIndexed(drm::IndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance);
	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance);

	virtual void Finish();

private:
	VulkanDevice& Device;
	VulkanAllocator& Allocator;
	VulkanDescriptorPool& DescriptorPool;

	bool bDirtyRenderPass = false;
	bool bDirtyPipelineLayout = false;
	bool bDirtyPipeline = false;
	bool bDirtyDescriptorSets = false;
	bool bDirtyVertexStreams = false;
	
	struct PendingState
	{
		RenderPassInitializer RPInit;
		PipelineStateInitializer PSOInit;
		VkDescriptorSetLayout DescriptorSetLayout;
		VkPipelineLayout PipelineLayout;
		VkPipeline Pipeline;
		std::vector<VulkanVertexBufferRef> VertexStreams;
	} Pending;

	PendingBuffer<VkRenderPass> RenderPasses;
	PendingBuffer<VkFramebuffer> Framebuffers;
	PendingBuffer<VkSampler> Samplers;

	template<typename VulkanDescriptorType>
	using DescriptorMap = HashTable<EShaderStage, HashTable<uint32, std::unique_ptr<VulkanDescriptorType>>>;
	DescriptorMap<VulkanWriteDescriptorImage> DescriptorImages;
	DescriptorMap<VulkanWriteDescriptorBuffer> DescriptorBuffers;

	void PrepareForDraw();
	void CleanRenderPass();
	void CleanDescriptorSets();
	void CleanVertexStreams();
};

CLASS(VulkanCommandList);
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

	virtual void SetRenderTargets(uint32 NumRTs, const drm::RenderTargetViewRef* ColorTargets, const drm::RenderTargetViewRef DepthTarget, EDepthStencilAccess Access);
	virtual void SetPipelineState(const PipelineStateInitializer& PSOInit);
	virtual void SetGraphicsPipeline(
		drm::ShaderRef Vertex,
		drm::ShaderRef TessControl,
		drm::ShaderRef TessEval,
		drm::ShaderRef Geometry,
		drm::ShaderRef Fragment
	);
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

	struct PendingGraphicsState
	{
		/** Render targets */
		uint32 NumRTs = 0;
		std::array<VulkanRenderTargetViewRef, PipelineStateInitializer::MaxSimultaneousRenderTargets> ColorTargets;
		VulkanRenderTargetViewRef DepthTarget;

		/** Graphics pipeline */
		GraphicsPipelineState GraphicsPipeline;

		/** Pipeline state */
		VkPipelineShaderStageCreateInfo			Stages{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		VkPipelineVertexInputStateCreateInfo	VertexInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		VkPipelineInputAssemblyStateCreateInfo	InputAssemblyState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		VkPipelineTessellationStateCreateInfo	TessellationState{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
		VkPipelineViewportStateCreateInfo		ViewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		VkPipelineRasterizationStateCreateInfo	RasterizationState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		VkPipelineMultisampleStateCreateInfo	MultisampleState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		VkPipelineDepthStencilStateCreateInfo	DepthStencilState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		VkPipelineColorBlendStateCreateInfo		ColorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		std::array<VkPipelineColorBlendAttachmentState, PipelineStateInitializer::MaxSimultaneousRenderTargets> ColorBlendAttachmentStates;
		VkPipelineDynamicStateCreateInfo		DynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		VkViewport								Viewport{};
		VkRect2D								Scissor{};

		std::vector<VulkanVertexBufferRef> VertexStreams;

		PendingGraphicsState(VulkanDevice& Device);
	} Pending;

	PendingBuffer<VkRenderPass> RenderPasses;
	PendingBuffer<VkFramebuffer> Framebuffers;
	PendingBuffer<VkDescriptorSetLayout> DescriptorSetLayouts;
	PendingBuffer<VkPipelineLayout> PipelineLayouts;
	PendingBuffer<VkPipeline> Pipelines;
	PendingBuffer<VkSampler> Samplers;

	template<typename VulkanDescriptorType>
	using DescriptorMap = HashTable<EShaderStage, HashTable<uint32, std::unique_ptr<VulkanDescriptorType>>>;

	// @todo-joe Why use a map? Just use an array for each descriptor.
	DescriptorMap<VulkanWriteDescriptorImage> DescriptorImages;
	DescriptorMap<VulkanWriteDescriptorBuffer> DescriptorBuffers;

	void PrepareForDraw();
	void CleanRenderPass();
	void CleanPipelineLayout();
	void CleanPipeline();
	void CleanDescriptorSets();
	void CleanVertexStreams();
};

CLASS(VulkanCommandList);
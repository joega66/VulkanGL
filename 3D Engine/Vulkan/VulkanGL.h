#pragma once
#include "../GL.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanMemory.h"
#include "VulkanDescriptors.h"
#include "VulkanShader.h"

class VulkanGL final : public GL
{
public:
	VulkanGL();

	virtual void InitGL();
	virtual void ReleaseGL();

	virtual void BeginRender();
	virtual void EndRender();

	virtual void SetRenderTargets(uint32 NumRTs, const GLRenderTargetViewRef* ColorTargets, const GLRenderTargetViewRef DepthTarget, EDepthStencilAccess Access);
	virtual void SetViewport(float X, float Y, float Width, float Height, float MinDepth = 0.0f, float MaxDepth = 1.0f);
	virtual void SetDepthTest(bool bDepthTestEnable, EDepthCompareTest CompareTest = Depth_Less);
	virtual void SetRasterizerState(ECullMode CullMode, EFrontFace FrontFace = FF_CCW, EPolygonMode PolygonMode = PM_Fill, float LineWidth = 1.0f);
	virtual void SetColorMask(uint32 RenderTargetIndex, EColorWriteMask ColorWriteMask);
	virtual void SetInputAssembly(EPrimitiveTopology Topology);
	virtual void SetGraphicsPipeline(
		GLShaderRef Vertex,
		GLShaderRef TessControl,
		GLShaderRef TessEval,
		GLShaderRef Geometry,
		GLShaderRef Fragment
	);
	virtual void SetVertexStream(uint32 Location, GLVertexBufferRef VertexBuffer);
	virtual void SetUniformBuffer(GLShaderRef Shader, uint32 Location, GLUniformBufferRef UniformBuffer);
	virtual void SetShaderImage(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState& Sampler);
	virtual void DrawIndexed(GLIndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance);
	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance);
	virtual GLIndexBufferRef CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsageFlags Usage, const void* Data = nullptr);
	virtual GLVertexBufferRef CreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsageFlags Usage, const void* Data = nullptr);
	virtual GLUniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate Usage = EUniformUpdate::Infrequent);
	virtual GLImageRef CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsageFlags UsageFlags, const uint8* Data);
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, float DepthClear, uint32 StencilClear);
	virtual GLRenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
	virtual void* LockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset);
	virtual void UnlockBuffer(GLVertexBufferRef VertexBuffer);
	virtual void* LockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset);
	virtual void UnlockBuffer(GLIndexBufferRef IndexBuffer);
	virtual void RebuildResolutionDependents();
	virtual std::string GetDeviceName() { return "Vulkan"; }

	VulkanDevice& GetDevice() { return Device; }
	//operator VkDevice() { return Device; }

	void CreateImageView(VulkanImageRef Image);

private:
	VulkanDevice Device;
	VulkanSurface Swapchain;
	VulkanAllocator Allocator;
	VulkanDescriptorPool DescriptorPool;
	std::vector<VkCommandBuffer> CommandBuffers;
	uint32 SwapchainIndex = -1;
	VkSemaphore ImageAvailableSem;
	VkSemaphore RenderEndSem;

	bool bDirtyRenderPass = false;
	bool bDirtyPipelineLayout = false;
	bool bDirtyPipeline = false;
	bool bDirtyDescriptorSets = false;
	bool bDirtyVertexStreams = false;

	enum EVulkanConstants
	{
		MaxSimultaneousRenderTargets = 8,
	};

	struct PendingGraphicsState
	{
		/** Render targets */
		uint32 NumRTs = 0;
		VulkanRenderTargetViewRef ColorTargets[MaxSimultaneousRenderTargets];
		VulkanRenderTargetViewRef DepthTarget;

		/** Graphics pipeline */
		VulkanShaderRef Vertex;
		VulkanShaderRef TessControl;
		VulkanShaderRef TessEval;
		VulkanShaderRef Geometry;
		VulkanShaderRef Fragment;
		VulkanShaderRef Compute;

		/** Pipeline state */
		VkPipelineShaderStageCreateInfo			Stages				{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		VkPipelineVertexInputStateCreateInfo	VertexInputState	{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		VkPipelineInputAssemblyStateCreateInfo	InputAssemblyState	{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		VkPipelineTessellationStateCreateInfo	TessellationState	{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
		VkPipelineViewportStateCreateInfo		ViewportState		{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		VkPipelineRasterizationStateCreateInfo	RasterizationState	{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		VkPipelineMultisampleStateCreateInfo	MultisampleState	{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		VkPipelineDepthStencilStateCreateInfo	DepthStencilState	{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		VkPipelineColorBlendStateCreateInfo		ColorBlendState =	{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		std::array<VkPipelineColorBlendAttachmentState, MaxSimultaneousRenderTargets> ColorBlendAttachments;
		VkPipelineDynamicStateCreateInfo		DynamicState		{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		VkViewport								Viewport			{};
		VkRect2D								Scissor				{};

		std::vector<VulkanVertexBufferRef> VertexStreams;

		PendingGraphicsState(VulkanDevice& Device);
		void SetDefaultPipeline(const VulkanDevice& Device);
		void ResetVertexStreams();
	} Pending;

	PendingBuffer<VkRenderPass> RenderPasses;
	PendingBuffer<VkFramebuffer> Framebuffers;
	PendingBuffer<VkDescriptorSetLayout> DescriptorSetLayouts;
	PendingBuffer<VkPipelineLayout> PipelineLayouts;
	PendingBuffer<VkPipeline> Pipelines;
	PendingBuffer<VkSampler> Samplers;
	PendingBuffer<VkDescriptorSet> DescriptorSets;

	template<typename VulkanDescriptorType>
	using DescriptorMap = Map<EShaderStage, Map<uint32, std::unique_ptr<VulkanDescriptorType>>>;

	DescriptorMap<VulkanWriteDescriptorImage> DescriptorImages;
	DescriptorMap<VulkanWriteDescriptorBuffer> DescriptorBuffers;

	VkCommandBuffer& GetCommandBuffer();
	VulkanRenderTargetViewRef GetCurrentSwapchainRTView();

	void PrepareForDraw();
	void CleanRenderPass();
	void CleanPipelineLayout();
	void CleanPipeline();
	void CleanDescriptorSets();

	void TransitionImageLayout(VulkanImageRef Image, VkImageLayout NewLayout, VkPipelineStageFlags DestinationStage);
	VkFormat FindSupportedDepthFormat(EImageFormat Format);
	void CreateImage(VkImage& Image, VkDeviceMemory& Memory, VkImageLayout& Layout, uint32 Width, uint32 Height, EImageFormat& Format, EResourceUsageFlags UsageFlags, const void* Data = nullptr);
	VkSampler CreateSampler(const SamplerState& Sampler);

	/** Engine conversions */
	const Map<EImageFormat, uint32> ImageFormatToGLSLSize;
};

CLASS(VulkanGL);
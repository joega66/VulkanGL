#pragma once
#include "GL.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanMemory.h"
#include "VulkanDescriptors.h"
#include "VulkanShader.h"

class VulkanGL : public GL
{
public:
	VulkanGL();

	virtual void InitGL() final;
	virtual void ReleaseGL() final;

	virtual void BeginRender() final;
	virtual void EndRender() final;

	virtual void SetRenderTargets(uint32 NumRTs, const GLRenderTargetViewRef* ColorTargets, const GLRenderTargetViewRef DepthTarget, EDepthStencilAccess Access) final;
	virtual void SetViewport(float X, float Y, float Width, float Height, float MinDepth = 0.0f, float MaxDepth = 1.0f) final;
	virtual void SetDepthTest(bool bDepthTestEnable, EDepthCompareTest CompareTest = Depth_Less) final;
	virtual void SetRasterizerState(ECullMode CullMode, EFrontFace FrontFace = FF_CCW, EPolygonMode PolygonMode = PM_Fill, float LineWidth = 1.0f) final;
	virtual void SetColorMask(uint32 RenderTargetIndex, EColorWriteMask ColorWriteMask) final;
	virtual void SetInputAssembly(EPrimitiveTopology Topology) final;
	virtual void SetGraphicsPipeline(
		GLShaderRef Vertex,
		GLShaderRef TessControl,
		GLShaderRef TessEval,
		GLShaderRef Geometry,
		GLShaderRef Fragment
	) final;
	virtual void SetShaderResource(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState& Sampler) final;
	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance) final;
	virtual GLImageRef CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceCreateFlags CreateFlags) final;
	virtual void ResizeImage(GLImageRef Image, uint32 Width, uint32 Height) final;
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) final;
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, float DepthClear, uint32 StencilClear) final;
	virtual GLRenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) final;
	virtual void RebuildResolutionDependents() final;
	virtual std::string GetDeviceName() final { return "Vulkan"; }

	VulkanDevice& GetDevice() { return Device; }
	operator VkDevice() { return Device; }

	void CreateImageView(VulkanImageRef Image);

private:
	VulkanDevice Device;
	VulkanSwapchain Swapchain;
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
	} Pending;

	PendingBuffer<VkRenderPass> RenderPasses;
	PendingBuffer<VkFramebuffer> Framebuffers;
	PendingBuffer<VkDescriptorSetLayout> DescriptorSetLayouts;
	PendingBuffer<VkPipelineLayout> PipelineLayouts;
	PendingBuffer<VkPipeline> Pipelines;
	PendingBuffer<VkSampler> Samplers;
	PendingBuffer<VkDescriptorSet> DescriptorSets;

	Map<EShaderStage, Map<uint32, VulkanWriteDescriptorImage>> DescriptorImages;

	VkCommandBuffer& GetCommandBuffer();
	VulkanRenderTargetViewRef GetCurrentSwapchainRTView();

	void CleanPipelineLayout();
	void CleanDescriptorSets();
	void CleanRenderPass();
	void CleanPipeline();

	void TransitionImageLayout(VulkanImageRef Image, VkImageLayout NewLayout);
	VkFormat FindSupportedDepthFormat(EImageFormat Format);
	void CreateImage(VkImage& Image, VkDeviceMemory& Memory, VkImageLayout& Layout, uint32 Width, uint32 Height, EImageFormat& Format, EResourceCreateFlags CreateFlags);
	VkSampler CreateSampler(const SamplerState& Sampler);
};

CLASS(VulkanGL);
#pragma once
#include "GPU/GPUResource.h"
#include "GPU/GPUShader.h"

#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanImage.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanSemaphore.h"

struct AttachmentView
{
	const gpu::Image* image = nullptr;
	ClearColorValue clearColor = {};
	ClearDepthStencilValue clearDepthStencil = {};
	ELoadAction loadAction = ELoadAction::DontCare;
	EStoreAction storeAction = EStoreAction::DontCare;
	EImageLayout initialLayout = EImageLayout::Undefined;
	EImageLayout finalLayout = EImageLayout::Undefined;

	AttachmentView() = default;

	AttachmentView(
		const gpu::Image* image,
		ELoadAction loadAction,
		EStoreAction storeAction,
		const ClearColorValue& clearValue,
		EImageLayout initialLayout,
		EImageLayout finalLayout)
		: image(image)
		, clearColor(clearValue)
		, loadAction(loadAction)
		, storeAction(storeAction)
		, initialLayout(initialLayout)
		, finalLayout(finalLayout)
	{
	}

	AttachmentView(
		const gpu::Image* image,
		ELoadAction loadAction,
		EStoreAction storeAction,
		const ClearDepthStencilValue& clearValue,
		EImageLayout initialLayout,
		EImageLayout finalLayout)
		: image(image)
		, clearDepthStencil(clearValue)
		, loadAction(loadAction)
		, storeAction(storeAction)
		, initialLayout(initialLayout)
		, finalLayout(finalLayout)
	{
	}
};

struct RenderArea
{
	glm::ivec2 offset = {};
	glm::uvec2 extent = {};
};

struct RenderPassDesc
{
	std::vector<AttachmentView>	colorAttachments;
	AttachmentView				depthAttachment;
	RenderArea					renderArea;
	EPipelineStage				srcStageMask;
	EPipelineStage				dstStageMask;
	EAccess						srcAccessMask;
	EAccess						dstAccessMask;
};

struct GraphicsPipelineDesc
{
	gpu::RenderPassView						renderPass;
	DepthStencilState						depthStencilState = {};
	RasterizationState						rasterizationState = {};
	MultisampleState						multisampleState = {};
	InputAssemblyState						inputAssemblyState = {};
	ShaderStages							shaderStages = {};
	SpecializationInfo						specInfo;
	std::vector<ColorBlendAttachmentState>	colorBlendAttachmentStates;
	std::vector<VertexAttributeDescription> vertexAttributes;
	std::vector<VertexBindingDescription>	vertexBindings;

	bool HasShader(const gpu::Shader* shader) const
	{
		switch (shader->compilationResult.stage)
		{
		case EShaderStage::Vertex:
			return shaderStages.vertex == shader;
		case EShaderStage::TessControl:
			return shaderStages.tessControl == shader;
		case EShaderStage::TessEvaluation:
			return shaderStages.tessEval == shader;
		case EShaderStage::Geometry:
			return shaderStages.geometry == shader;
		default: // EShaderStage::Fragment
			return shaderStages.fragment == shader;
		}
	}
};

struct ComputePipelineDesc
{
	const gpu::Shader* shader = nullptr;
	SpecializationInfo specInfo;
};

struct BufferMemoryBarrier
{
	const gpu::Buffer& buffer;
	EAccess srcAccessMask;
	EAccess dstAccessMask;
};

struct ImageMemoryBarrier
{
	const gpu::Image& image;
	EAccess srcAccessMask;
	EAccess dstAccessMask;
	EImageLayout oldLayout;
	EImageLayout newLayout;
	uint32 baseMipLevel = 0;
	uint32 levelCount = 1;
};

#include "Vulkan/VulkanCommandBuffer.h"
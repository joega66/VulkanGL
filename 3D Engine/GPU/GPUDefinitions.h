#pragma once
#include "GPU/GPUResource.h"
#include "GPU/GPUShader.h"

#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanImage.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanPipeline.h"

namespace gpu
{
	using Pipeline = std::shared_ptr<VulkanPipeline>;
}

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
	AttachmentView					depthAttachment;
	RenderArea							renderArea;
	EPipelineStage						srcStageMask;
	EPipelineStage						dstStageMask;
	EAccess								srcAccessMask;
	EAccess								dstAccessMask;
};

struct PipelineStateDesc
{
	gpu::RenderPassView renderPass;
	Scissor scissor;
	Viewport viewport;
	DepthStencilState depthStencilState;
	RasterizationState rasterizationState;
	MultisampleState multisampleState;
	InputAssemblyState inputAssemblyState;
	ShaderStages shaderStages;
	SpecializationInfo specInfo;
	std::vector<ColorBlendAttachmentState> colorBlendAttachmentStates;
	std::vector<EDynamicState> dynamicStates;
	std::vector<VertexAttributeDescription> vertexAttributes;
	std::vector<VertexBindingDescription> vertexBindings;
	
	friend bool operator==(const PipelineStateDesc& l, const PipelineStateDesc& r)
	{
		return l.renderPass == r.renderPass
			&& l.scissor == r.scissor
			&& l.viewport == r.viewport
			&& l.depthStencilState == r.depthStencilState
			&& l.rasterizationState == r.rasterizationState
			&& l.multisampleState == r.multisampleState
			&& l.inputAssemblyState == r.inputAssemblyState
			&& l.shaderStages == r.shaderStages
			&& l.specInfo == r.specInfo
			&& l.colorBlendAttachmentStates == r.colorBlendAttachmentStates
			&& l.dynamicStates == r.dynamicStates
			&& l.vertexAttributes == r.vertexAttributes
			&& l.vertexBindings == r.vertexBindings;
	}

	bool HasShader(const gpu::Shader* shader) const
	{
		switch (shader->compilationInfo.stage)
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

namespace std
{
	template<> struct hash<PipelineStateDesc>
	{
		std::size_t operator()(PipelineStateDesc const& psoDesc) const noexcept
		{
			std::size_t seed = 0;
			HashCombine(seed, psoDesc.renderPass.GetRenderPass());
			HashCombine(seed, Platform::CalculateCrc(&psoDesc.scissor, sizeof(psoDesc.scissor)));
			HashCombine(seed, Platform::CalculateCrc(&psoDesc.viewport, sizeof(psoDesc.viewport)));
			HashCombine(seed, Platform::CalculateCrc(&psoDesc.depthStencilState, sizeof(psoDesc.depthStencilState)));
			HashCombine(seed, Platform::CalculateCrc(&psoDesc.rasterizationState, sizeof(psoDesc.rasterizationState)));
			HashCombine(seed, Platform::CalculateCrc(&psoDesc.multisampleState, sizeof(psoDesc.multisampleState)));
			HashCombine(seed, Platform::CalculateCrc(&psoDesc.inputAssemblyState, sizeof(psoDesc.inputAssemblyState)));
			HashCombine(seed, Platform::CalculateCrc(&psoDesc.shaderStages, sizeof(psoDesc.shaderStages)));
			HashCombine(seed, Platform::CalculateCrc(psoDesc.specInfo.GetMapEntries().data(), psoDesc.specInfo.GetMapEntries().size() * sizeof(SpecializationInfo::SpecializationMapEntry)));
			HashCombine(seed, Platform::CalculateCrc(psoDesc.specInfo.GetData().data(), psoDesc.specInfo.GetData().size()));
			HashCombine(seed, Platform::CalculateCrc(psoDesc.colorBlendAttachmentStates.data(), psoDesc.colorBlendAttachmentStates.size() * sizeof(ColorBlendAttachmentState)));
			HashCombine(seed, Platform::CalculateCrc(psoDesc.dynamicStates.data(), psoDesc.dynamicStates.size() * sizeof(EDynamicState)));
			HashCombine(seed, Platform::CalculateCrc(psoDesc.vertexAttributes.data(), psoDesc.vertexAttributes.size() * sizeof(VertexAttributeDescription)));
			HashCombine(seed, Platform::CalculateCrc(psoDesc.vertexBindings.data(), psoDesc.vertexBindings.size() * sizeof(VertexBindingDescription)));
			return seed;
		}
	};
}

struct ComputePipelineDesc
{
	const gpu::Shader* computeShader = nullptr;
	SpecializationInfo specInfo;

	inline bool operator==(const ComputePipelineDesc& other) const
	{
		return computeShader == other.computeShader
			&& specInfo == other.specInfo;
	}
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

#include "Vulkan/VulkanCommandList.h"
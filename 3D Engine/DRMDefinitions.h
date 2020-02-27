#pragma once
#include <DRMResource.h>
#include <DRMShader.h>

#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanImage.h"
#include "Vulkan/VulkanMemory.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanPipeline.h"

namespace drm
{
	using Buffer = VulkanBuffer;
	using BufferView = VulkanBufferView;
	using Image = VulkanImage;
	using ImageView = VulkanImageView;
	using Sampler = VulkanSampler;
	using DescriptorSet = VulkanDescriptorSet;
	using DescriptorSetLayout = VulkanDescriptorSetLayout;
	using Pipeline = VulkanPipeline;
	using RenderPass = VulkanRenderPass;
	using RenderPassView = VulkanRenderPassView;

	class AttachmentView
	{
	public:
		const drm::Image* Image = nullptr;
		std::variant<ClearColorValue, ClearDepthStencilValue> ClearValue;
		ELoadAction LoadAction = ELoadAction::DontCare;
		EStoreAction StoreAction = EStoreAction::DontCare;
		EImageLayout InitialLayout = EImageLayout::Undefined;
		EImageLayout FinalLayout = EImageLayout::Undefined;

		AttachmentView() = default;

		AttachmentView(const drm::Image* Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout InitialLayout, EImageLayout FinalLayout)
			: Image(Image)
			, LoadAction(LoadAction)
			, StoreAction(StoreAction)
			, ClearValue(ClearValue)
			, InitialLayout(InitialLayout)
			, FinalLayout(FinalLayout)
		{
		}

		AttachmentView(const drm::Image* Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil, EImageLayout InitialLayout, EImageLayout FinalLayout)
			: Image(Image)
			, LoadAction(LoadAction)
			, StoreAction(StoreAction)
			, ClearValue(DepthStencil)
			, InitialLayout(InitialLayout)
			, FinalLayout(FinalLayout)
		{
		}

		friend bool operator==(const AttachmentView& L, const AttachmentView& R)
		{
			return L.LoadAction == R.LoadAction
				&& L.StoreAction == R.StoreAction
				&& L.InitialLayout == R.InitialLayout
				&& L.FinalLayout == R.FinalLayout;
		}
	};
}

struct RenderArea
{
	glm::ivec2 Offset;
	glm::uvec2 Extent;
};

struct RenderPassDesc
{
	std::vector<drm::AttachmentView> ColorAttachments;
	drm::AttachmentView DepthAttachment;
	RenderArea RenderArea;

	friend bool operator==(const RenderPassDesc& L, const RenderPassDesc& R)
	{
		return L.DepthAttachment == R.DepthAttachment
			&& L.ColorAttachments == R.ColorAttachments;
	}
};

struct PipelineStateDesc
{
	drm::RenderPassView RenderPass;
	ScissorDesc Scissor;
	Viewport Viewport;
	DepthStencilState DepthStencilState;
	RasterizationState RasterizationState;
	MultisampleState MultisampleState;
	std::vector<ColorBlendAttachmentState> ColorBlendAttachmentStates;
	InputAssemblyState InputAssemblyState;
	ShaderStages ShaderStages;
	SpecializationInfo SpecializationInfo;
	std::vector<EDynamicState> DynamicStates;
	std::vector<VertexAttributeDescription> VertexAttributes;
	std::vector<VertexBindingDescription> VertexBindings;
	std::vector<const drm::DescriptorSet*> DescriptorSets;

	friend bool operator==(const PipelineStateDesc& L, const PipelineStateDesc& R)
	{
		return L.RenderPass == R.RenderPass
			&& L.Scissor == R.Scissor
			&& L.Viewport == R.Viewport
			&& L.DepthStencilState == R.DepthStencilState
			&& L.RasterizationState == R.RasterizationState
			&& L.MultisampleState == R.MultisampleState
			&& L.ColorBlendAttachmentStates == R.ColorBlendAttachmentStates
			&& L.InputAssemblyState == R.InputAssemblyState
			&& L.ShaderStages == R.ShaderStages
			&& L.SpecializationInfo == R.SpecializationInfo
			&& L.DynamicStates == R.DynamicStates
			&& L.VertexAttributes == R.VertexAttributes
			&& L.VertexBindings == R.VertexBindings;
	}

	bool HasShader(const drm::Shader* Shader) const
	{
		switch (Shader->CompilationInfo.Stage)
		{
		case EShaderStage::Vertex:
			return ShaderStages.Vertex == Shader;
		case EShaderStage::TessControl:
			return ShaderStages.TessControl == Shader;
		case EShaderStage::TessEvaluation:
			return ShaderStages.TessEval == Shader;
		case EShaderStage::Geometry:
			return ShaderStages.Geometry == Shader;
		default: // EShaderStage::Fragment
			return ShaderStages.Fragment == Shader;
		}
	}
};

struct ComputePipelineDesc
{
	const drm::Shader* ComputeShader;
	SpecializationInfo SpecializationInfo;
	std::vector<const drm::DescriptorSet*> DescriptorSets;

	inline bool operator==(const ComputePipelineDesc& Other) const
	{
		return ComputeShader == Other.ComputeShader
			&& SpecializationInfo == Other.SpecializationInfo;
	}
};

struct BufferMemoryBarrier
{
	const drm::Buffer& Buffer;
	EAccess SrcAccessMask;
	EAccess DstAccessMask;

	BufferMemoryBarrier(const drm::Buffer& Buffer, EAccess SrcAccessMask, EAccess DstAccessMask)
		: Buffer(Buffer), SrcAccessMask(SrcAccessMask), DstAccessMask(DstAccessMask)
	{
	}
};

struct ImageMemoryBarrier
{
	const drm::Image& Image;
	EAccess SrcAccessMask;
	EAccess DstAccessMask;
	EImageLayout OldLayout;
	EImageLayout NewLayout;

	ImageMemoryBarrier(const drm::Image& Image, EAccess SrcAccessMask, EAccess DstAccessMask, EImageLayout OldLayout, EImageLayout NewLayout)
		: Image(Image), SrcAccessMask(SrcAccessMask), DstAccessMask(DstAccessMask), OldLayout(OldLayout), NewLayout(NewLayout)
	{
	}

	ImageMemoryBarrier(const ImageMemoryBarrier& Other)
		: Image(Other.Image), SrcAccessMask(Other.SrcAccessMask), DstAccessMask(Other.DstAccessMask), OldLayout(Other.OldLayout), NewLayout(Other.NewLayout)
	{
	}
};

#include "Vulkan/VulkanCommandList.h"

namespace drm
{
	using CommandList = VulkanCommandList;
}
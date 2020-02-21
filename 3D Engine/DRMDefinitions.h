#pragma once
#include <DRMResource.h>

#include "Vulkan/VulkanRenderPass.h"

namespace drm
{
	using RenderPass = VulkanRenderPass;
}

struct PipelineStateDesc
{
	const drm::RenderPass* RenderPass;
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

	friend bool operator==(const PipelineStateDesc& L, const PipelineStateDesc& R)
	{
		return L.Scissor == R.Scissor
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
};

#include "Vulkan/VulkanImage.h"
#include "Vulkan/VulkanMemory.h"
#include "Vulkan/VulkanDescriptors.h"

namespace drm
{
	using Buffer = VulkanBuffer;
	using Image = VulkanImage;
	using Sampler = VulkanSampler;
	using DescriptorSet = VulkanDescriptorSet;
	using DescriptorSetLayout = VulkanDescriptorSetLayout;

	class AttachmentView
	{
	public:
		const drm::Image* Image = nullptr; // @todo ref
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

		friend bool operator!=(const AttachmentView& L, const AttachmentView& R)
		{
			return !(L == R);
		}
	};
}

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
};

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
		if (L.ColorAttachments.size() != R.ColorAttachments.size())
			return false;

		if (L.DepthAttachment != R.DepthAttachment)
			return false;

		for (uint32 ColorAttachmentIndex = 0; ColorAttachmentIndex < L.ColorAttachments.size(); ColorAttachmentIndex++)
		{
			if (L.ColorAttachments[ColorAttachmentIndex] != R.ColorAttachments[ColorAttachmentIndex])
				return false;
		}

		return true;
	}
};

#include "Vulkan/VulkanCommandList.h"

namespace drm
{
	using CommandList = VulkanCommandList;
}
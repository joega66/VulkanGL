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

#include "Vulkan/VulkanCommandList.h"

namespace drm
{
	using CommandList = VulkanCommandList;
}
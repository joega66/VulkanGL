#pragma once
#include <DRMResource.h>
#include <DRMShader.h>

#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanImage.h"
#include "Vulkan/VulkanMemory.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanBindlessResources.h"
#include "Vulkan/VulkanPipeline.h"

namespace drm
{
	using Buffer = VulkanBuffer;
	using DescriptorBufferInfo = VulkanDescriptorBufferInfo;
	using Image = VulkanImage;
	using ImageView = VulkanImageView;
	using DescriptorImageInfo = VulkanDescriptorImageInfo;
	using Sampler = VulkanSampler;
	using DescriptorSet = VulkanDescriptorSet;
	using DescriptorSetLayout = VulkanDescriptorSetLayout;
	using Pipeline = std::shared_ptr<VulkanPipeline>;
	using RenderPass = VulkanRenderPass;
	using RenderPassView = VulkanRenderPassView;
	using BindlessResources = VulkanBindlessResources;
	using TextureID = VulkanTextureID;
	using SamplerID = VulkanSamplerID;

	class AttachmentView
	{
	public:
		const drm::Image* Image = nullptr;
		ClearColorValue ClearColor = {};
		ClearDepthStencilValue ClearDepthStencil = {};
		ELoadAction LoadAction = ELoadAction::DontCare;
		EStoreAction StoreAction = EStoreAction::DontCare;
		EImageLayout InitialLayout = EImageLayout::Undefined;
		EImageLayout FinalLayout = EImageLayout::Undefined;

		AttachmentView() = default;

		AttachmentView(const drm::Image* Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout InitialLayout, EImageLayout FinalLayout)
			: Image(Image)
			, ClearColor(ClearValue)
			, LoadAction(LoadAction)
			, StoreAction(StoreAction)
			, InitialLayout(InitialLayout)
			, FinalLayout(FinalLayout)
		{
		}

		AttachmentView(const drm::Image* Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& ClearValue, EImageLayout InitialLayout, EImageLayout FinalLayout)
			: Image(Image)
			, ClearDepthStencil(ClearValue)
			, LoadAction(LoadAction)
			, StoreAction(StoreAction)
			, InitialLayout(InitialLayout)
			, FinalLayout(FinalLayout)
		{
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
};

struct PushConstantRange
{
	EShaderStage StageFlags = EShaderStage::All;
	uint32 Size = 0;

	bool operator==(const PushConstantRange& Other) const
	{
		return StageFlags == Other.StageFlags
			&& Size == Other.Size;
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
	InputAssemblyState InputAssemblyState;
	ShaderStages ShaderStages;
	SpecializationInfo SpecializationInfo;
	std::vector<ColorBlendAttachmentState> ColorBlendAttachmentStates;
	std::vector<EDynamicState> DynamicStates;
	std::vector<VertexAttributeDescription> VertexAttributes;
	std::vector<VertexBindingDescription> VertexBindings;
	std::vector<VkDescriptorSetLayout> Layouts;
	PushConstantRange PushConstantRange;

	friend bool operator==(const PipelineStateDesc& L, const PipelineStateDesc& R)
	{
		return L.RenderPass == R.RenderPass
			&& L.Scissor == R.Scissor
			&& L.Viewport == R.Viewport
			&& L.DepthStencilState == R.DepthStencilState
			&& L.RasterizationState == R.RasterizationState
			&& L.MultisampleState == R.MultisampleState
			&& L.InputAssemblyState == R.InputAssemblyState
			&& L.ShaderStages == R.ShaderStages
			&& L.SpecializationInfo == R.SpecializationInfo
			&& L.ColorBlendAttachmentStates == R.ColorBlendAttachmentStates
			&& L.DynamicStates == R.DynamicStates
			&& L.VertexAttributes == R.VertexAttributes
			&& L.VertexBindings == R.VertexBindings
			&& L.Layouts == R.Layouts
			&& L.PushConstantRange == R.PushConstantRange;
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

namespace std
{
	template<> struct hash<PipelineStateDesc>
	{
		std::size_t operator()(PipelineStateDesc const& PSODesc) const noexcept
		{
			std::size_t Seed = 0;
			HashCombine(Seed, PSODesc.RenderPass.GetRenderPass());
			HashCombine(Seed, Platform::CalculateCrc(&PSODesc.Scissor, sizeof(PSODesc.Scissor)));
			HashCombine(Seed, Platform::CalculateCrc(&PSODesc.Viewport, sizeof(PSODesc.Viewport)));
			HashCombine(Seed, Platform::CalculateCrc(&PSODesc.DepthStencilState, sizeof(PSODesc.DepthStencilState)));
			HashCombine(Seed, Platform::CalculateCrc(&PSODesc.RasterizationState, sizeof(PSODesc.RasterizationState)));
			HashCombine(Seed, Platform::CalculateCrc(&PSODesc.MultisampleState, sizeof(PSODesc.MultisampleState)));
			HashCombine(Seed, Platform::CalculateCrc(&PSODesc.InputAssemblyState, sizeof(PSODesc.InputAssemblyState)));
			HashCombine(Seed, Platform::CalculateCrc(&PSODesc.ShaderStages, sizeof(PSODesc.ShaderStages)));
			HashCombine(Seed, Platform::CalculateCrc(PSODesc.SpecializationInfo.GetMapEntries().data(), PSODesc.SpecializationInfo.GetMapEntries().size() * sizeof(SpecializationInfo::SpecializationMapEntry)));
			HashCombine(Seed, Platform::CalculateCrc(PSODesc.SpecializationInfo.GetData().data(), PSODesc.SpecializationInfo.GetData().size()));
			HashCombine(Seed, Platform::CalculateCrc(PSODesc.ColorBlendAttachmentStates.data(), PSODesc.ColorBlendAttachmentStates.size() * sizeof(ColorBlendAttachmentState)));
			HashCombine(Seed, Platform::CalculateCrc(PSODesc.DynamicStates.data(), PSODesc.DynamicStates.size() * sizeof(EDynamicState)));
			HashCombine(Seed, Platform::CalculateCrc(PSODesc.VertexAttributes.data(), PSODesc.VertexAttributes.size() * sizeof(VertexAttributeDescription)));
			HashCombine(Seed, Platform::CalculateCrc(PSODesc.VertexBindings.data(), PSODesc.VertexBindings.size() * sizeof(VertexBindingDescription)));
			HashCombine(Seed, Platform::CalculateCrc(PSODesc.Layouts.data(), PSODesc.Layouts.size() * sizeof(VkDescriptorSetLayout)));
			HashCombine(Seed, Platform::CalculateCrc(&PSODesc.PushConstantRange, sizeof(PSODesc.PushConstantRange)));
			return Seed;
		}
	};
}

struct ComputePipelineDesc
{
	const drm::Shader* ComputeShader = nullptr;
	SpecializationInfo SpecializationInfo;
	std::vector<VkDescriptorSetLayout> Layouts;
	PushConstantRange PushConstantRange;

	inline bool operator==(const ComputePipelineDesc& Other) const
	{
		return ComputeShader == Other.ComputeShader
			&& SpecializationInfo == Other.SpecializationInfo
			&& Layouts == Other.Layouts
			&& PushConstantRange == Other.PushConstantRange;
	}
};

struct BufferMemoryBarrier
{
	const drm::Buffer& Buffer;
	EAccess SrcAccessMask;
	EAccess DstAccessMask;
};

struct ImageMemoryBarrier
{
	const drm::Image& Image;
	EAccess SrcAccessMask;
	EAccess DstAccessMask;
	EImageLayout OldLayout;
	EImageLayout NewLayout;
	uint32 BaseMipLevel = 0;
	uint32 LevelCount = 1;
};

#include "Vulkan/VulkanCommandList.h"

namespace drm
{
	using CommandList = VulkanCommandList;
}
#pragma once
#include "DRMShader.h"
#include "DRMResource.h"

struct Viewport
{
	int32 X = 0;
	int32 Y = 0;
	int32 Width = 0;
	int32 Height = 0;
	float MinDepth = 0.0f;
	float MaxDepth = 1.0f;

	friend bool operator==(const Viewport& L, const Viewport& R)
	{
		return L.X == R.X
			&& L.Y == R.Y
			&& L.Width == R.Width
			&& L.Height == R.Height
			&& L.MinDepth == R.MinDepth
			&& L.MaxDepth == R.MaxDepth;
	}
};

struct StencilOpState
{
	EStencilOp FailOp = EStencilOp::Keep;
	EStencilOp PassOp = EStencilOp::Replace;
	EStencilOp DepthFailOp = EStencilOp::Keep;
	ECompareOp CompareOp = ECompareOp::Always;
	uint32 CompareMask = 0;
	uint32 WriteMask = 0;
	uint32 Reference = 0;

	friend bool operator==(const StencilOpState& L, const StencilOpState& R)
	{
		return L.FailOp == R.FailOp
			&& L.PassOp == R.PassOp
			&& L.DepthFailOp == R.DepthFailOp
			&& L.CompareOp == R.CompareOp
			&& L.CompareMask == R.CompareMask
			&& L.WriteMask == R.WriteMask
			&& L.Reference == R.Reference;
	}
};

struct DepthStencilState
{
	bool DepthTestEnable = true;
	bool DepthWriteEnable = true;
	EDepthCompareTest DepthCompareTest = EDepthCompareTest::LEqual;
	bool DepthBoundsTestEnable = false;
	bool StencilTestEnable = false;
	StencilOpState Front;
	StencilOpState Back;
	float MinDepthBounds = 0.0f;
	float MaxDepthBounds = 0.0f;

	friend bool operator==(const DepthStencilState& L, const DepthStencilState& R)
	{
		return L.DepthTestEnable == R.DepthTestEnable
			&& L.DepthWriteEnable == R.DepthWriteEnable
			&& L.DepthCompareTest == R.DepthCompareTest
			&& L.DepthBoundsTestEnable == R.DepthBoundsTestEnable
			&& L.StencilTestEnable == R.StencilTestEnable
			&& L.Front == R.Front
			&& L.Back == R.Back
			&& L.MinDepthBounds == R.MinDepthBounds
			&& L.MaxDepthBounds == R.MaxDepthBounds;
	}
};

struct RasterizationState
{
	bool DepthClampEnable = false;
	bool RasterizerDiscardEnable = false;
	EPolygonMode PolygonMode = EPolygonMode::Fill;
	ECullMode CullMode = ECullMode::None;
	EFrontFace FrontFace = EFrontFace::CW;
	bool DepthBiasEnable = false;
	float DepthBiasConstantFactor = 0.0f;
	float DepthBiasClamp = 0.0f;
	float DepthBiasSlopeFactor = 0.0f;
	float LineWidth = 1.0f;

	friend bool operator==(const RasterizationState& L, const RasterizationState& R)
	{
		return L.DepthClampEnable == R.DepthClampEnable
			&& L.RasterizerDiscardEnable == R.RasterizerDiscardEnable
			&& L.PolygonMode == R.PolygonMode
			&& L.CullMode == R.CullMode
			&& L.FrontFace == R.FrontFace
			&& L.DepthBiasEnable == R.DepthBiasEnable
			&& L.DepthBiasConstantFactor == R.DepthBiasConstantFactor
			&& L.DepthBiasClamp == R.DepthBiasClamp
			&& L.DepthBiasSlopeFactor == R.DepthBiasSlopeFactor
			&& L.LineWidth == R.LineWidth;
	}
};

enum ESampleCount
{
	None = 0,
	Samples1 = 0x01,
	Samples2 = 0x02,
	Samples4 = 0x04,
	Samples8 = 0x08,
	Samples16 = 0x10,
	Samples32 = 0x20,
	Samples64 = 0x40
};

struct MultisampleState
{
	ESampleCount RasterizationSamples = ESampleCount::Samples1;
	bool SampleShadingEnable = false;
	float MinSampleShading = 0.0f;
	bool AlphaToCoverageEnable = false;
	bool AlphaToOneEnable = false;

	friend bool operator==(const MultisampleState& L, const MultisampleState& R)
	{
		return L.RasterizationSamples == R.RasterizationSamples
			&& L.SampleShadingEnable == R.SampleShadingEnable
			&& L.MinSampleShading == R.MinSampleShading
			&& L.AlphaToCoverageEnable == R.AlphaToCoverageEnable
			&& L.AlphaToOneEnable == R.AlphaToOneEnable;
	}
};

enum class EBlendFactor
{
	ZERO = 0,
	ONE = 1,
	SRC_COLOR = 2,
	ONE_MINUS_SRC_COLOR = 3,
	DST_COLOR = 4,
	ONE_MINUS_DST_COLOR = 5,
	SRC_ALPHA = 6,
	ONE_MINUS_SRC_ALPHA = 7,
	DST_ALPHA = 8,
	ONE_MINUS_DST_ALPHA = 9,
	CONSTANT_COLOR = 10,
	ONE_MINUS_CONSTANT_COLOR = 11,
	CONSTANT_ALPHA = 12,
	ONE_MINUS_CONSTANT_ALPHA = 13,
	SRC_ALPHA_SATURATE = 14,
	SRC1_COLOR = 15,
	ONE_MINUS_SRC1_COLOR = 16,
	SRC1_ALPHA = 17,
	ONE_MINUS_SRC1_ALPHA = 18,
};

enum class EBlendOp
{
	ADD = 0,
	SUBTRACT = 1,
	REVERSE_SUBTRACT = 2,
	MIN = 3,
	MAX = 4,
};

struct ColorBlendAttachmentState
{
	bool BlendEnable = false;
	EBlendFactor SrcColorBlendFactor;
	EBlendFactor DstColorBlendFactor;
	EBlendOp ColorBlendOp;
	EBlendFactor SrcAlphaBlendFactor;
	EBlendFactor DstAlphaBlendFactor;
	EBlendOp AlphaBlendOp;
	EColorChannel ColorWriteMask = EColorChannel::RGBA;

	friend bool operator==(const ColorBlendAttachmentState& L, const ColorBlendAttachmentState& R)
	{
		return L.BlendEnable == R.BlendEnable
			&& L.SrcColorBlendFactor == R.SrcColorBlendFactor
			&& L.DstColorBlendFactor == R.DstColorBlendFactor
			&& L.ColorBlendOp == R.ColorBlendOp
			&& L.SrcAlphaBlendFactor == R.SrcAlphaBlendFactor
			&& L.DstAlphaBlendFactor == R.DstAlphaBlendFactor
			&& L.AlphaBlendOp == R.AlphaBlendOp
			&& L.ColorWriteMask == R.ColorWriteMask;
	}

	friend bool operator!=(const ColorBlendAttachmentState& L, const ColorBlendAttachmentState& R)
	{
		return !(L == R);
	}
};

struct InputAssemblyState
{
	EPrimitiveTopology Topology = EPrimitiveTopology::TriangleList;
	bool PrimitiveRestartEnable = false;

	friend bool operator==(const InputAssemblyState& L, const InputAssemblyState& R)
	{
		return L.Topology == R.Topology && L.PrimitiveRestartEnable == R.PrimitiveRestartEnable;
	}
};

struct GraphicsPipelineState
{
	drm::ShaderRef Vertex;
	drm::ShaderRef TessControl;
	drm::ShaderRef TessEval;
	drm::ShaderRef Geometry;
	drm::ShaderRef Fragment;

	friend bool operator==(const GraphicsPipelineState& L, const GraphicsPipelineState& R)
	{
		return L.Vertex == R.Vertex && L.TessControl == R.TessControl && L.TessEval == R.TessEval 
			&& L.Geometry == R.Geometry && L.Fragment == R.Fragment;
	}

	friend bool operator!=(const GraphicsPipelineState& L, const GraphicsPipelineState& R)
	{
		return !(L == R);
	}
};

struct RenderArea
{
	glm::ivec2 Offset;
	glm::uvec2 Extent;
};

enum
{
	MaxRenderTargets = 8
};

struct RenderPassInitializer
{
	uint32 NumRenderTargets;
	std::array<drm::RenderTargetViewRef, MaxRenderTargets> ColorTargets;
	drm::RenderTargetViewRef DepthTarget;
	RenderArea RenderArea;
};

struct PipelineStateInitializer
{
	Viewport Viewport;
	DepthStencilState DepthStencilState;
	RasterizationState RasterizationState;
	MultisampleState MultisampleState;
	std::array<ColorBlendAttachmentState, MaxRenderTargets> ColorBlendAttachmentStates;
	InputAssemblyState InputAssemblyState;
	GraphicsPipelineState GraphicsPipelineState;
	std::array<SpecializationInfo, NumGraphicsStages> SpecializationInfos;

	friend bool operator==(const PipelineStateInitializer& L, const PipelineStateInitializer& R)
	{
		for (uint32 RenderTargetIndex = 0; RenderTargetIndex < MaxRenderTargets; RenderTargetIndex++)
		{
			if (L.ColorBlendAttachmentStates[RenderTargetIndex] != R.ColorBlendAttachmentStates[RenderTargetIndex])
			{
				return false;
			}
		}

		return L.Viewport == R.Viewport
			&& L.DepthStencilState == R.DepthStencilState
			&& L.RasterizationState == R.RasterizationState
			&& L.MultisampleState == R.MultisampleState
			&& L.InputAssemblyState == R.InputAssemblyState
			&& L.GraphicsPipelineState == R.GraphicsPipelineState
			&& L.SpecializationInfos == R.SpecializationInfos;
	}
};

class RenderCommandList
{
public:
	virtual void BeginRenderPass(const RenderPassInitializer& RenderPassInit) = 0;
	virtual void EndRenderPass() = 0;
	virtual void BindPipeline(const PipelineStateInitializer& PSOInit) = 0;
	virtual void BindDescriptorSets(uint32 NumDescriptorSets, const drm::DescriptorSetRef* DescriptorSets) = 0;
	virtual void BindVertexBuffers(uint32 NumVertexBuffers, const drm::BufferRef* VertexBuffers) = 0;
	virtual void DrawIndexed(drm::BufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance) = 0;
	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance) = 0;
	virtual void Finish() = 0;
	virtual void ClearColorImage(drm::ImageRef Image, const ClearColorValue& Color) = 0;
	virtual void PipelineBarrier(drm::ImageRef Image, EImageLayout NewLayout, EAccess DstAccessMask, EPipelineStage DstStageMask) = 0;
};

CLASS(RenderCommandList);
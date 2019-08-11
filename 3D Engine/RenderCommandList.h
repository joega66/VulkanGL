#pragma once
#include "DRMShader.h"
#include "DRMResource.h"

enum class EDepthStencilTransition
{
	None,
	// No transition.
	DepthWriteStencilWrite,
	// Transitions depth to shader read for the depth aspect.
	DepthReadStencilWrite,
	// Transitions depth to shader read for the stencil aspect.
	DepthWriteStencilRead,
	// Transitions depth to shader read for depth/stencil aspect.
	DepthReadStencilRead,
};

enum class EDepthCompareTest
{
	Never,
	Less,
	Equal,
	LEqual,
	Greater,
	NEqual,
	GEqual,
	Always
};

enum class EPolygonMode
{
	Fill,
	Line,
	Point
};

enum class EFrontFace
{
	CCW,
	CW
};

enum class ECullMode
{
	None,
	Front,
	Back,
	FrontAndBack
};

enum class EColorChannel
{
	None,
	R = 0x01,
	G = 0x02,
	B = 0x04,
	A = 0x08,
	RGBA = R | G | B | A
};

enum class EPrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	TriangleFan
};

enum class ESamplerAddressMode
{
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder,
	MirrorClampToEdge
};

enum class ESamplerMipmapMode
{
	Nearest,
	Linear
};

enum class EFilter
{
	Nearest,
	Linear,
	Cubic
};

struct SamplerState
{
	EFilter Filter = EFilter::Linear;
	ESamplerAddressMode SAM = ESamplerAddressMode::ClampToEdge;
	ESamplerMipmapMode SMM = ESamplerMipmapMode::Linear;
};

enum class EUniformUpdate
{
	Infrequent,
	Frequent,
};

enum class EStencilOp
{
	Keep,
	Zero,
	Replace,
};

enum class ECompareOp
{
	Never,
	Less,
	Equal,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual,
	Always
};

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
};

enum class EBlendOp
{
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

struct RenderPassInitializer
{
	enum
	{
		MaxRenderTargets = 8
	};

	uint32 NumRenderTargets = 0;
	std::array<drm::RenderTargetViewRef, MaxRenderTargets> ColorTargets;
	drm::RenderTargetViewRef DepthTarget;
	EDepthStencilTransition DepthStencilTransition;

	friend bool operator==(const RenderPassInitializer& L, const RenderPassInitializer& R)
	{
		// Compare depth attachments.
		if (!L.DepthTarget && !R.DepthTarget)
		{
		}
		else if (L.DepthTarget && R.DepthTarget)
		{
			if (!(*L.DepthTarget == *R.DepthTarget && L.DepthStencilTransition == R.DepthStencilTransition))
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		// Compare color attachments.
		if (L.NumRenderTargets == R.NumRenderTargets)
		{
			for (uint32 RenderTargetIndex = 0; RenderTargetIndex < L.NumRenderTargets; RenderTargetIndex++)
			{
				if (!(*L.ColorTargets[RenderTargetIndex] == *R.ColorTargets[RenderTargetIndex]))
				{
					return false;
				}
			}
		}
		else
		{
			return false;
		}

		return true;
	}
};

struct PipelineStateInitializer
{
	Viewport Viewport;
	DepthStencilState DepthStencilState;
	RasterizationState RasterizationState;
	MultisampleState MultisampleState;
	std::array<ColorBlendAttachmentState, RenderPassInitializer::MaxRenderTargets> ColorBlendAttachmentStates;
	InputAssemblyState InputAssemblyState;
	GraphicsPipelineState GraphicsPipelineState;

	friend bool operator==(const PipelineStateInitializer& L, const PipelineStateInitializer& R)
	{
		for (uint32 RenderTargetIndex = 0; RenderTargetIndex < RenderPassInitializer::MaxRenderTargets; RenderTargetIndex++)
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
			&& L.GraphicsPipelineState == R.GraphicsPipelineState;
	}
};

class RenderCommandList
{
public:
	virtual void BeginRenderPass(const RenderPassInitializer& RenderPassInit) = 0;
	virtual void BindPipeline(const PipelineStateInitializer& PSOInit) = 0;
	virtual void BindVertexBuffers(uint32 Location, drm::VertexBufferRef VertexBuffer) = 0;
	virtual void SetUniformBuffer(drm::ShaderRef Shader, uint32 Location, drm::UniformBufferRef UniformBuffer) = 0;
	virtual void SetShaderImage(drm::ShaderRef Shader, uint32 Location, drm::ImageRef Image, const SamplerState& Sampler) = 0;
	virtual void SetStorageBuffer(drm::ShaderRef Shader, uint32 Location, drm::StorageBufferRef StorageBuffer) = 0;
	virtual void DrawIndexed(drm::IndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance) = 0;
	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance) = 0;
	virtual void Finish() = 0;
};

CLASS(RenderCommandList);
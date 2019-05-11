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
	ESamplerAddressMode SAM = ESamplerAddressMode::ClampToBorder;
	ESamplerMipmapMode SMM = ESamplerMipmapMode::Linear;
};

enum class EUniformUpdate
{
	// Device local.
	Infrequent,
	// Use CPU-accessible strategy.
	Frequent,
	// Use CPU-accessible strategy.
	SingleFrame
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
	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
	float MinDepth = 0.0f;
	float MaxDepth = 1.0f;
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
	const uint32* SampleMask = nullptr;
	bool AlphaToCoverageEnable = false;
	bool AlphaToOneEnable = false;
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
};

struct InputAssemblyState
{
	EPrimitiveTopology Topology = EPrimitiveTopology::TriangleList;
	bool PrimitiveRestartEnable = false;
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
		MaxSimultaneousRenderTargets = 8
	};

	uint32 NumRenderTargets = 0;
	std::array<drm::RenderTargetViewRef, MaxSimultaneousRenderTargets> ColorTargets;
	drm::RenderTargetViewRef DepthTarget;
	EDepthStencilTransition DepthStencilTransition;

	friend bool operator==(const RenderPassInitializer& L, const RenderPassInitializer& R)
	{
		if (L.NumRenderTargets == R.NumRenderTargets
			&& L.DepthTarget == R.DepthTarget
			&& L.DepthStencilTransition == R.DepthStencilTransition)
		{
			for (uint32 RenderTargetIndex = 0; RenderTargetIndex < L.NumRenderTargets; RenderTargetIndex++)
			{
				if (L.ColorTargets[RenderTargetIndex] != R.ColorTargets[RenderTargetIndex])
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
	std::array<ColorBlendAttachmentState, RenderPassInitializer::MaxSimultaneousRenderTargets> ColorBlendAttachmentStates;
	InputAssemblyState InputAssemblyState;
	GraphicsPipelineState GraphicsPipelineState;
};

class RenderCommandList
{
public:
	virtual void SetRenderTargets(const RenderPassInitializer& RenderPassInit) = 0;
	virtual void SetPipelineState(const PipelineStateInitializer& PSOInit) = 0;
	virtual void SetVertexStream(uint32 Location, drm::VertexBufferRef VertexBuffer) = 0;
	virtual void SetUniformBuffer(drm::ShaderRef Shader, uint32 Location, drm::UniformBufferRef UniformBuffer) = 0;
	virtual void SetShaderImage(drm::ShaderRef Shader, uint32 Location, drm::ImageRef Image, const SamplerState& Sampler) = 0;
	virtual void SetStorageBuffer(drm::ShaderRef Shader, uint32 Location, drm::StorageBufferRef StorageBuffer) = 0;
	virtual void DrawIndexed(drm::IndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance) = 0;
	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance) = 0;

	virtual void Finish() = 0;
};

CLASS(RenderCommandList);
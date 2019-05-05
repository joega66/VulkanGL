#pragma once
#include <Platform/Platform.h>
#include "GLShader.h"

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
	GLShaderRef Vertex;
	GLShaderRef TessControl;
	GLShaderRef TessEval;
	GLShaderRef Geometry;
	GLShaderRef Fragment;
};

struct PipelineStateInitializer
{
	enum
	{
		MaxSimultaneousRenderTargets = 8
	};

	Viewport Viewport;
	DepthStencilState DepthStencilState;
	RasterizationState RasterizationState;
	std::array<ColorBlendAttachmentState, MaxSimultaneousRenderTargets> ColorBlendAttachmentStates;
	InputAssemblyState InputAssemblyState;
	GraphicsPipelineState GraphicsPipelineState;
};

/** Graphics Library Interface */
class RenderCommandList : public GLRenderResource
{
public:
	virtual void Init() = 0;
	virtual void Release() = 0;

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	virtual void SetRenderTargets(uint32 NumRTs, const GLRenderTargetViewRef* ColorTargets, const GLRenderTargetViewRef DepthTarget, EDepthStencilAccess Access) = 0;
	virtual void SetPipelineState(const PipelineStateInitializer& PSOInit) = 0;
	virtual void SetGraphicsPipeline(
		GLShaderRef Vertex,
		GLShaderRef TessControl,
		GLShaderRef TessEval,
		GLShaderRef Geometry,
		GLShaderRef Fragment
	) = 0;
	virtual void SetVertexStream(uint32 Location, GLVertexBufferRef VertexBuffer) = 0;
	virtual void SetUniformBuffer(GLShaderRef Shader, uint32 Location, GLUniformBufferRef UniformBuffer) = 0;
	virtual void SetShaderImage(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState& Sampler) = 0;
	virtual void SetStorageBuffer(GLShaderRef Shader, uint32 Location, GLStorageBufferRef StorageBuffer) = 0;
	virtual void DrawIndexed(GLIndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance) = 0;
	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance) = 0;
	virtual GLIndexBufferRef CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void* Data = nullptr) = 0;
	virtual GLVertexBufferRef CreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void* Data = nullptr) = 0;
	virtual GLUniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate Usage = EUniformUpdate::Infrequent) = 0;
	virtual GLStorageBufferRef CreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage) = 0;
	virtual GLImageRef CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data = nullptr) = 0;
	virtual GLImageRef CreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo) = 0;
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) = 0;
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil) = 0;
	virtual GLRenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) = 0;
	virtual void* LockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset) = 0;
	virtual void UnlockBuffer(GLVertexBufferRef VertexBuffer) = 0;
	virtual void* LockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset) = 0;
	virtual void UnlockBuffer(GLIndexBufferRef IndexBuffer) = 0;
	virtual void RebuildResolutionDependents() = 0;
	virtual std::string GetDeviceName() = 0;
};

CLASS(RenderCommandList);

extern RenderCommandListRef GRenderCmdList;

GLIndexBufferRef GLCreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void* Data = nullptr);
GLVertexBufferRef GLCreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void* Data = nullptr);
template<typename UniformBufferType>
GLUniformBufferRef GLCreateUniformBuffer(EUniformUpdate Usage = EUniformUpdate::Infrequent);
template<typename UniformBufferType>
GLUniformBufferRef GLCreateUniformBuffer(const UniformBufferType& Data, EUniformUpdate Usage = EUniformUpdate::Infrequent);
GLStorageBufferRef GLCreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage = EResourceUsage::None);
GLImageRef GLCreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data = nullptr);
GLImageRef GLCreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo);
GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil);
GLRenderTargetViewRef GLGetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
void* GLLockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset);
void GLUnlockBuffer(GLVertexBufferRef VertexBuffer);
void* GLLockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset);
void GLUnlockBuffer(GLIndexBufferRef IndexBuffer);
void GLRebuildResolutionDependents();
std::string GLGetDeviceName();

template<typename ShaderType>
GLShaderRef GLCreateShader()
{
	if (GLShaderRef Shader = GShaderCompiler->FindShader(std::type_index(typeid(ShaderType))); Shader)
	{
		return Shader;
	}
	else
	{
		const auto& [Filename, EntryPoint, Stage] = ShaderType::GetBaseShaderInfo();
		return GShaderCompiler->CompileShader<ShaderType>(Filename, EntryPoint, Stage);
	}
}

template<typename UniformBufferType>
inline GLUniformBufferRef GLCreateUniformBuffer(EUniformUpdate Usage)
{
	return GRenderCmdList->CreateUniformBuffer(sizeof(UniformBufferType), nullptr, Usage);
}

template<typename UniformBufferType>
inline GLUniformBufferRef GLCreateUniformBuffer(const UniformBufferType & Data, EUniformUpdate Usage)
{
	return GRenderCmdList->CreateUniformBuffer(sizeof(UniformBufferType), &Data, Usage);
}
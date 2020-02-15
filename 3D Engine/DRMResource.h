#pragma once
#include <Platform/Platform.h>

enum class EFormat
{
	UNDEFINED,
	R8_UNORM,
	R8_UINT,
	R8_SINT,
	R8_SRGB,
	R8G8_UINT,
	R8G8_SINT,
	R8G8_SRGB,
	R8G8B8_UINT,
	R8G8B8_SINT,
	R8G8B8_SRGB,
	B8G8R8_UINT,
	B8G8R8_SINT,
	B8G8R8_SRGB,
	R8G8B8A8_UINT,
	R8G8B8A8_SINT,
	R8G8B8A8_SRGB,
	R8G8B8A8_UNORM,
	B8G8R8A8_UINT,
	B8G8R8A8_SINT,
	B8G8R8A8_SRGB,
	B8G8R8A8_UNORM,
	R16_UINT,
	R16_SINT,
	R16_SFLOAT,
	R16_UNORM,
	R16G16_UINT,
	R16G16_SINT,
	R16G16_SFLOAT,
	R16G16B16_UINT,
	R16G16B16_SINT,
	R16G16B16_SFLOAT,
	R16G16B16A16_UINT,
	R16G16B16A16_SINT,
	R16G16B16A16_SFLOAT,
	R32_UINT,
	R32_SINT,
	R32_SFLOAT,
	R32_UNORM,
	R32G32_UINT,
	R32G32_SINT,
	R32G32_SFLOAT,
	R32G32B32_UINT,
	R32G32B32_SINT,
	R32G32B32_SFLOAT,
	R32G32B32A32_UINT,
	R32G32B32A32_SINT,
	R32G32B32A32_SFLOAT,
	D16_UNORM,
	D32_SFLOAT,
	S8_UINT,
	D32_SFLOAT_S8_UINT,
	D24_UNORM_S8_UINT,
	BC2_UNORM_BLOCK,
};

struct DrawIndirectCommand
{
	uint32 VertexCount;
	uint32 InstanceCount;
	uint32 FirstVertex;
	uint32 FirstInstance;
};

enum class EAccess
{
	None = 0,
	IndirectCommandRead = 0x00000001,
	IndexRead = 0x00000002,
	VertexAttributeRead = 0x00000004,
	UniformRead = 0x00000008,
	InputAttachmentRead = 0x00000010,
	ShaderRead = 0x00000020,
	ShaderWrite = 0x00000040,
	ColorAttachmentRead = 0x00000080,
	ColorAttachmentWrite = 0x00000100,
	DepthStencilAttachmentRead = 0x00000200,
	DepthStencilAttachmentWrite = 0x00000400,
	TransferRead = 0x00000800,
	TransferWrite = 0x00001000,
	HostRead = 0x00002000,
	HostWrite = 0x00004000,
	MemoryRead = 0x00008000,
	MemoryWrite = 0x00010000,
};

enum class EPipelineStage
{
	None = 0,
	TopOfPipe = 0x00000001,
	DrawIndirect = 0x00000002,
	VertexInput = 0x00000004,
	VertexShader = 0x00000008,
	TessellationControlShader = 0x00000010,
	TessellationEvaluationShader = 0x00000020,
	GeometryShader = 0x00000040,
	FragmentShader = 0x00000080,
	EarlyFragmentTests = 0x00000100,
	LateFragmentTests = 0x00000200,
	ColorAttachmentOutput = 0x00000400,
	ComputeShader = 0x00000800,
	Transfer = 0x00001000,
	BottomOfPipe = 0x00002000,
	Host = 0x00004000,
	AllGraphics = 0x00008000,
	AllCommands = 0x00010000,
};

enum class EBufferUsage
{
	None,
	Indirect = 1 << 0,
	KeepCPUAccessible = 1 << 1,
	Vertex = 1 << 2,
	Storage = 1 << 3,
	Index = 1 << 4,
	Uniform = 1 << 5,
	Transfer = 1 << 6,
};

enum class EImageUsage
{
	None,
	Attachment	= 1 << 0,
	Sampled		= 1 << 1,
	Cubemap		= 1 << 2,
	Storage		= 1 << 3,
	TransferSrc = 1 << 4,
	TransferDst	= 1 << 5,
};

enum class ELoadAction
{
	DontCare,
	Clear,
	Load
};

enum class EStoreAction
{
	DontCare,
	Store
};

struct ClearDepthStencilValue
{
	float	DepthClear = 1.0f;
	uint32	StencilClear = 0;
};

struct ClearColorValue
{
	float	Float32[4];
	int32	Int32[4];
	uint32	Uint32[4];
};

enum class EImageLayout
{
	Undefined,
	General,
	ColorAttachmentOptimal,
	DepthWriteStencilWrite,
	DepthReadStencilRead,
	ShaderReadOnlyOptimal,
	TransferSrcOptimal,
	TransferDstOptimal,
	Preinitialized,
	DepthReadStencilWrite,
	DepthWriteStencilRead,
	Present
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

	bool operator==(const SamplerState& Other) const
	{
		return Filter == Other.Filter && SAM == Other.SAM && SMM == Other.SMM;
	}

	struct Hash
	{
		size_t operator()(const SamplerState& SamplerState) const
		{
			return ((uint32)SamplerState.Filter * 31) + ((uint32)SamplerState.SAM * 37) + ((uint32)SamplerState.SMM * 41);
		}
	};
};

enum EDescriptorType
{
	SampledImage,
	StorageImage,
	UniformBuffer,
	StorageBuffer,
};

struct DescriptorTemplateEntry
{
	uint32 Binding;
	uint32 DescriptorCount;
	EDescriptorType DescriptorType;
};

namespace drm
{
	class Buffer
	{
	public:
		EBufferUsage Usage;

		Buffer(EBufferUsage Usage, uint32 Size)
			: Usage(Usage)
			, Size(Size)
		{
		}

		virtual ~Buffer() {}

		inline uint32 GetSize() const { return Size; }

	private:
		uint32 Size;

	};
	
	CLASS(Buffer);

	class Image
	{
	public:
		EFormat Format;
		uint32 Width;
		uint32 Height;
		uint32 Depth;
		EImageUsage Usage;

		Image(EFormat Format, uint32 Width, uint32 Height, uint32 Depth, EImageUsage UsageFlags)
			: Format(Format), Width(Width), Height(Height), Depth(Depth), Usage(UsageFlags)
		{
		}

		virtual ~Image() {}

		virtual uint64 GetNativeHandle() = 0;

		bool IsColor() const;

		bool IsStencil() const;

		bool IsDepthStencil() const;

		bool IsDepth() const;

		uint32 GetStrideInBytes() const;

		inline float64 GetAspect() const { return static_cast<float64>(Width) / static_cast<float64>(Height); }

		inline uint32 GetSize() const 
		{ 
			return Width * Height * Depth * GetStrideInBytes() * (Any(Usage & EImageUsage::Cubemap) ? 6 : 1); 
		}

		static bool IsDepthStencil(EFormat Format);

		static bool IsDepth(EFormat Format);

		static uint32 GetSize(EFormat Format);
	};

	CLASS(Image);

	class AttachmentView
	{
	public:
		ImageRef Image = nullptr;
		std::variant<ClearColorValue, ClearDepthStencilValue> ClearValue;
		ELoadAction LoadAction = ELoadAction::DontCare;
		EStoreAction StoreAction = EStoreAction::DontCare;
		EImageLayout InitialLayout = EImageLayout::Undefined;
		EImageLayout FinalLayout = EImageLayout::Undefined;

		AttachmentView() = default;
		AttachmentView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout InitialLayout, EImageLayout FinalLayout);
		AttachmentView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil, EImageLayout InitialLayout, EImageLayout FinalLayout);

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

	class DescriptorSet
	{
	public:
		virtual ~DescriptorSet() {}
	};

	CLASS(DescriptorSet);

	class DescriptorTemplate
	{
	public:
		virtual ~DescriptorTemplate() {}
		virtual drm::DescriptorSetRef CreateDescriptorSet() = 0;
		virtual void UpdateDescriptorSet(drm::DescriptorSetRef DescriptorSet, void* Data) = 0;
	};

	CLASS(DescriptorTemplate);
}

/** drm::CommandList */
struct RenderArea
{
	glm::ivec2 Offset;
	glm::uvec2 Extent;
};

struct RenderPassDesc
{
	enum
	{
		MaxAttachments = 8
	};

	uint32 NumAttachments;
	std::array<drm::AttachmentView, MaxAttachments> ColorAttachments;
	drm::AttachmentView DepthAttachment;
	RenderArea RenderArea;

	friend bool operator==(const RenderPassDesc& L, const RenderPassDesc& R)
	{
		if (L.NumAttachments != R.NumAttachments)
			return false;

		if (L.DepthAttachment != R.DepthAttachment)
			return false;

		for (uint32 ColorAttachmentIndex = 0; ColorAttachmentIndex < L.NumAttachments; ColorAttachmentIndex++)
		{
			if (L.ColorAttachments[ColorAttachmentIndex] != R.ColorAttachments[ColorAttachmentIndex])
				return false;
		}

		return true;
	}
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

struct ScissorDesc
{
	glm::ivec2 Offset = glm::ivec2(0);
	glm::uvec2 Extent = glm::uvec2(0);

	friend bool operator==(const ScissorDesc& L, const ScissorDesc& R)
	{
		return L.Offset == R.Offset
			&& L.Extent == R.Extent;
	}
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

enum class EColorChannel
{
	None,
	R = 0x01,
	G = 0x02,
	B = 0x04,
	A = 0x08,
	RGBA = R | G | B | A
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

enum class EPrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	TriangleFan
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

namespace drm { class Shader; }

struct ShaderStages
{
	const drm::Shader* Vertex;
	const drm::Shader* TessControl;
	const drm::Shader* TessEval;
	const drm::Shader* Geometry;
	const drm::Shader* Fragment;

	friend bool operator==(const ShaderStages& L, const ShaderStages& R)
	{
		return L.Vertex == R.Vertex && L.TessControl == R.TessControl && L.TessEval == R.TessEval
			&& L.Geometry == R.Geometry && L.Fragment == R.Fragment;
	}

	friend bool operator!=(const ShaderStages& L, const ShaderStages& R)
	{
		return !(L == R);
	}
};

enum class EDynamicState
{
	Viewport = 0,
	Scissor = 1,
	LineWidth = 2,
	DepthBias = 3,
	BlendConstants = 4,
	DepthBounds = 5,
	StencilCompareMask = 6,
	StencilWriteMask = 7,
	StencilReference = 8,
};

class SpecializationInfo
{
public:
	struct SpecializationMapEntry
	{
		uint32 ConstantID;
		uint32 Offset;
		size_t Size;

		bool operator==(const SpecializationMapEntry& Other) const
		{
			return ConstantID == Other.ConstantID
				&& Offset == Other.Offset
				&& Size == Other.Size;
		}
	};

	SpecializationInfo() = default;

	template<typename SpecializationConstantType>
	void Add(uint32 ConstantID, const SpecializationConstantType& Constant)
	{
		const uint32 Offset = Data.size();
		Data.resize(Offset + sizeof(Constant));
		memcpy(Data.data() + Offset, &Constant, sizeof(Constant));
		const SpecializationMapEntry MapEntry = { ConstantID, Offset, sizeof(Constant) };
		MapEntries.push_back(std::move(MapEntry));
	}

	bool operator==(const SpecializationInfo& Other) const
	{
		return MapEntries == Other.MapEntries
			&& Data == Other.Data;
	}

	inline const std::vector<SpecializationMapEntry>& GetMapEntries() const { return MapEntries; }

	inline const std::vector<uint8>& GetData() const { return Data; }

private:
	std::vector<SpecializationMapEntry> MapEntries;
	std::vector<uint8> Data;
};

struct VertexAttributeDescription
{
	uint64	Location;
	uint32	Binding;
	EFormat	Format;
	uint32	Offset;

	friend bool operator==(const VertexAttributeDescription& L, const VertexAttributeDescription& R)
	{
		return L.Location == R.Location
			&& L.Binding == R.Binding
			&& L.Format == R.Format
			&& L.Offset == R.Offset;
	}
};

struct VertexBindingDescription
{
	uint32 Binding;
	uint32 Stride;

	friend bool operator==(const VertexBindingDescription& L, const VertexBindingDescription& R)
	{
		return L.Binding == R.Binding
			&& L.Stride == R.Stride;
	}
};

struct PipelineStateDesc
{
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

struct BufferMemoryBarrier
{
	drm::BufferRef Buffer;
	EAccess SrcAccessMask;
	EAccess DstAccessMask;

	BufferMemoryBarrier(drm::BufferRef Buffer, EAccess SrcAccessMask, EAccess DstAccessMask)
		: Buffer(Buffer), SrcAccessMask(SrcAccessMask), DstAccessMask(DstAccessMask)
	{
	}
};

struct ImageMemoryBarrier
{
	drm::ImageRef Image;
	EAccess SrcAccessMask;
	EAccess DstAccessMask;
	EImageLayout OldLayout;
	EImageLayout NewLayout;

	ImageMemoryBarrier(drm::ImageRef Image, EAccess SrcAccessMask, EAccess DstAccessMask, EImageLayout OldLayout, EImageLayout NewLayout)
		: Image(Image), SrcAccessMask(SrcAccessMask), DstAccessMask(DstAccessMask), OldLayout(OldLayout), NewLayout(NewLayout)
	{
	}
};

enum class EIndexType
{
	UINT16,
	UINT32
};

enum class EQueue
{
	Graphics,
	Compute,
	Transfer,
};
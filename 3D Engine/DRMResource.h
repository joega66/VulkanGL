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
	uint32 vertexCount;
	uint32 instanceCount;
	uint32 firstVertex;
	uint32 firstInstance;
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
}; ENABLE_BITWISE_OPERATORS(EAccess)

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
}; ENABLE_BITWISE_OPERATORS(EPipelineStage);

enum class EBufferUsage
{
	None,
	Indirect = 1 << 0,
	HostVisible = 1 << 1,
	Vertex = 1 << 2,
	Storage = 1 << 3,
	Index = 1 << 4,
	Uniform = 1 << 5,
	Transfer = 1 << 6,
}; ENABLE_BITWISE_OPERATORS(EBufferUsage);

enum class EImageUsage
{
	None,
	Attachment	= 1 << 0,
	Sampled		= 1 << 1,
	Cubemap		= 1 << 2,
	Storage		= 1 << 3,
	TransferSrc = 1 << 4,
	TransferDst	= 1 << 5,
}; ENABLE_BITWISE_OPERATORS(EImageUsage);

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
	float	depthClear = 1.0f;
	uint32	stencilClear = 0;
};

struct ClearColorValue
{
	float	float32[4]	= { 0.0f, 0.0f, 0.0f, 0.0f };
	int32	int32[4]	= { 0, 0, 0, 0 };
	uint32	uint32[4]	= { 0, 0, 0, 0 };
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

struct SamplerDesc
{
	EFilter filter = EFilter::Linear;
	ESamplerAddressMode sam = ESamplerAddressMode::ClampToEdge;
	ESamplerMipmapMode smm = ESamplerMipmapMode::Linear;
	float minLod = 0.0f;
	float maxLod = 0.0f;
};

enum class EDescriptorType
{
	SampledImage,
	StorageImage,
	UniformBuffer,
	StorageBuffer,
};

struct DescriptorBinding
{
	uint32 binding;
	uint32 descriptorCount;
	EDescriptorType descriptorType;
};

namespace drm
{
	class BufferPrivate
	{
	public:
		BufferPrivate() = default;
		BufferPrivate(EBufferUsage usage, uint32 size)
			: _Usage(usage)
			, _Size(size)
		{
		}

		inline EBufferUsage GetUsage() const { return _Usage; }
		inline uint32 GetSize() const { return _Size; }

	protected:
		EBufferUsage _Usage;
		uint32 _Size;
	};

	class ImagePrivate
	{
	public:
		ImagePrivate() = default;
		ImagePrivate(EFormat format, uint32 width, uint32 height, uint32 depth, EImageUsage usage, uint32 mipLevels)
			: _Format(format), _Width(width), _Height(height), _Depth(depth), _Usage(usage), _MipLevels(mipLevels)
		{
		}

		inline EFormat GetFormat() const { return _Format; }
		inline uint32 GetWidth() const { return _Width; }
		inline uint32 GetHeight() const { return _Height; }
		inline uint32 GetDepth() const { return _Depth; }
		inline EImageUsage GetUsage() const { return _Usage; }
		inline uint32 GetMipLevels() const { return _MipLevels; }

		bool IsColor() const;
		bool IsStencil() const;
		bool IsDepthStencil() const;
		bool IsDepth() const;
		uint32 GetStrideInBytes() const;
		inline float64 GetAspect() const { return static_cast<float64>(_Width) / static_cast<float64>(_Height); }
		inline uint32 GetSize() const 
		{ 
			return _Width * _Height * _Depth * GetStrideInBytes() * (Any(_Usage & EImageUsage::Cubemap) ? 6 : 1);
		}

		static bool IsColor(EFormat format);
		static bool IsStencil(EFormat format);
		static bool IsDepthStencil(EFormat format);
		static bool IsDepth(EFormat format);
		static uint32 GetSize(EFormat format);

	protected:
		EFormat _Format;
		uint32 _Width;
		uint32 _Height;
		uint32 _Depth;
		EImageUsage _Usage;
		uint32 _MipLevels;
	};
}

struct Viewport
{
	int32 x = 0;
	int32 y = 0;
	int32 width = 0;
	int32 height = 0;
	float minDepth = 0.0f;
	float maxDepth = 1.0f;

	friend bool operator==(const Viewport& l, const Viewport& r)
	{
		return l.x == r.x
			&& l.y == r.y
			&& l.width == r.width
			&& l.height == r.height
			&& l.minDepth == r.minDepth
			&& l.maxDepth == r.maxDepth;
	}
};

struct Scissor
{
	glm::ivec2 offset = glm::ivec2(0);
	glm::uvec2 extent = glm::uvec2(0);

	friend bool operator==(const Scissor& l, const Scissor& r)
	{
		return l.offset == r.offset
			&& l.extent == r.extent;
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
	EStencilOp failOp = EStencilOp::Keep;
	EStencilOp passOp = EStencilOp::Replace;
	EStencilOp depthFailOp = EStencilOp::Keep;
	ECompareOp compareOp = ECompareOp::Always;
	uint32 compareMask = 0;
	uint32 writeMask = 0;
	uint32 reference = 0;

	friend bool operator==(const StencilOpState& l, const StencilOpState& r)
	{
		return l.failOp == r.failOp
			&& l.passOp == r.passOp
			&& l.depthFailOp == r.depthFailOp
			&& l.compareOp == r.compareOp
			&& l.compareMask == r.compareMask
			&& l.writeMask == r.writeMask
			&& l.reference == r.reference;
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
	bool depthTestEnable = true;
	bool depthWriteEnable = true;
	EDepthCompareTest depthCompareTest = EDepthCompareTest::LEqual;
	bool depthBoundsTestEnable = false;
	bool stencilTestEnable = false;
	StencilOpState front = {};
	StencilOpState back = {};
	float minDepthBounds = 0.0f;
	float maxDepthBounds = 0.0f;

	friend bool operator==(const DepthStencilState& l, const DepthStencilState& r)
	{
		return l.depthTestEnable == r.depthTestEnable
			&& l.depthWriteEnable == r.depthWriteEnable
			&& l.depthCompareTest == r.depthCompareTest
			&& l.depthBoundsTestEnable == r.depthBoundsTestEnable
			&& l.stencilTestEnable == r.stencilTestEnable
			&& l.front == r.front
			&& l.back == r.back
			&& l.minDepthBounds == r.minDepthBounds
			&& l.maxDepthBounds == r.maxDepthBounds;
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
	bool depthClampEnable = false;
	bool rasterizerDiscardEnable = false;
	EPolygonMode polygonMode = EPolygonMode::Fill;
	ECullMode cullMode = ECullMode::None;
	EFrontFace frontFace = EFrontFace::CW;
	bool depthBiasEnable = false;
	float depthBiasConstantFactor = 0.0f;
	float depthBiasClamp = 0.0f;
	float depthBiasSlopeFactor = 0.0f;
	float lineWidth = 1.0f;

	friend bool operator==(const RasterizationState& l, const RasterizationState& r)
	{
		return l.depthClampEnable == r.depthClampEnable
			&& l.rasterizerDiscardEnable == r.rasterizerDiscardEnable
			&& l.polygonMode == r.polygonMode
			&& l.cullMode == r.cullMode
			&& l.frontFace == r.frontFace
			&& l.depthBiasEnable == r.depthBiasEnable
			&& l.depthBiasConstantFactor == r.depthBiasConstantFactor
			&& l.depthBiasClamp == r.depthBiasClamp
			&& l.depthBiasSlopeFactor == r.depthBiasSlopeFactor
			&& l.lineWidth == r.lineWidth;
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
	ESampleCount rasterizationSamples = ESampleCount::Samples1;
	bool sampleShadingEnable = false;
	float minSampleShading = 0.0f;
	bool alphaToCoverageEnable = false;
	bool alphaToOneEnable = false;

	friend bool operator==(const MultisampleState& l, const MultisampleState& r)
	{
		return l.rasterizationSamples == r.rasterizationSamples
			&& l.sampleShadingEnable == r.sampleShadingEnable
			&& l.minSampleShading == r.minSampleShading
			&& l.alphaToCoverageEnable == r.alphaToCoverageEnable
			&& l.alphaToOneEnable == r.alphaToOneEnable;
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
}; ENABLE_BITWISE_OPERATORS(EColorChannel);

struct ColorBlendAttachmentState
{
	bool blendEnable = false;
	EBlendFactor srcColorBlendFactor;
	EBlendFactor dstColorBlendFactor;
	EBlendOp colorBlendOp;
	EBlendFactor srcAlphaBlendFactor;
	EBlendFactor dstAlphaBlendFactor;
	EBlendOp alphaBlendOp;
	EColorChannel colorWriteMask = EColorChannel::RGBA;

	friend bool operator==(const ColorBlendAttachmentState& l, const ColorBlendAttachmentState& r)
	{
		return l.blendEnable == r.blendEnable
			&& l.srcColorBlendFactor == r.srcColorBlendFactor
			&& l.dstColorBlendFactor == r.dstColorBlendFactor
			&& l.colorBlendOp == r.colorBlendOp
			&& l.srcAlphaBlendFactor == r.srcAlphaBlendFactor
			&& l.dstAlphaBlendFactor == r.dstAlphaBlendFactor
			&& l.alphaBlendOp == r.alphaBlendOp
			&& l.colorWriteMask == r.colorWriteMask;
	}

	friend bool operator!=(const ColorBlendAttachmentState& l, const ColorBlendAttachmentState& r)
	{
		return !(l == r);
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
	EPrimitiveTopology topology = EPrimitiveTopology::TriangleList;
	bool primitiveRestartEnable = false;

	friend bool operator==(const InputAssemblyState& l, const InputAssemblyState& r)
	{
		return l.topology == r.topology 
			&& l.primitiveRestartEnable == r.primitiveRestartEnable;
	}
};

namespace drm { class Shader; }

struct ShaderStages
{
	const drm::Shader* vertex = nullptr;
	const drm::Shader* tessControl = nullptr;
	const drm::Shader* tessEval = nullptr;
	const drm::Shader* geometry = nullptr;
	const drm::Shader* fragment = nullptr;

	friend bool operator==(const ShaderStages& l, const ShaderStages& r)
	{
		return l.vertex == r.vertex 
			&& l.tessControl == r.tessControl 
			&& l.tessEval == r.tessEval
			&& l.geometry == r.geometry 
			&& l.fragment == r.fragment;
	}

	friend bool operator!=(const ShaderStages& l, const ShaderStages& r)
	{
		return !(l == r);
	}
};

enum class EShaderStage
{
	None = 0,
	Vertex = 1 << 0,
	TessControl = 1 << 1,
	TessEvaluation = 1 << 2,
	Geometry = 1 << 3,
	Fragment = 1 << 4,
	Compute = 1 << 5,
	AllGraphics = Vertex | TessControl | TessEvaluation | Geometry | Fragment,
	All = AllGraphics | Compute
}; ENABLE_BITWISE_OPERATORS(EShaderStage);

struct PushConstantRange
{
	EShaderStage stageFlags = EShaderStage::All;
	uint32 offset = 0;
	uint32 size = 0;

	bool operator==(const PushConstantRange& other) const
	{
		return stageFlags == other.stageFlags
			&& offset == other.offset
			&& size == other.size;
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
		uint32 constantID;
		uint32 offset;
		size_t size;

		bool operator==(const SpecializationMapEntry& other) const;
	};

	SpecializationInfo() = default;

	template<typename SpecializationConstantType>
	void Add(uint32 constantID, const SpecializationConstantType& constant)
	{
		const uint32 offset = (uint32)_Data.size();
		_Data.resize(offset + sizeof(constant));
		memcpy(_Data.data() + offset, &constant, sizeof(constant));
		const SpecializationMapEntry mapEntry = { constantID, offset, sizeof(constant) };
		_MapEntries.push_back(std::move(mapEntry));
	}

	bool operator==(const SpecializationInfo& other) const;

	inline const std::vector<SpecializationMapEntry>& GetMapEntries() const { return _MapEntries; }

	inline const std::vector<uint8>& GetData() const { return _Data; }

private:
	std::vector<SpecializationMapEntry> _MapEntries;
	std::vector<uint8> _Data;
};

struct VertexAttributeDescription
{
	uint64	location;
	uint32	binding;
	EFormat	format;
	uint32	offset;

	friend bool operator==(const VertexAttributeDescription& l, const VertexAttributeDescription& r)
	{
		return l.location == r.location
			&& l.binding == r.binding
			&& l.format == r.format
			&& l.offset == r.offset;
	}
};

struct VertexBindingDescription
{
	uint32 binding;
	uint32 stride;

	friend bool operator==(const VertexBindingDescription& l, const VertexBindingDescription& r)
	{
		return l.binding == r.binding
			&& l.stride == r.stride;
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
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

		Buffer(EBufferUsage Usage)
			: Usage(Usage)
		{
		}

		virtual ~Buffer() {}
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
		virtual void Write(ImageRef Image, const SamplerState& Sampler, uint32 Binding) = 0;
		virtual void Write(ImageRef Image, uint32 Binding) = 0;
		virtual void Write(BufferRef Buffer, uint32 Binding) = 0;
		virtual void Update() = 0;
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

	class RenderPass
	{
	public:
		virtual ~RenderPass() {}
	};

	CLASS(RenderPass);
}
#pragma once
#include <Platform/Platform.h>

enum class EFormat
{
	UNDEFINED,
	R8_UINT,
	R8_SINT,
	R8G8_UINT,
	R8G8_SINT,
	R8_SRGB,
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

enum class EResourceUsage
{
	None,
	RenderTargetable = 1 << 0,
	ShaderResource = 1 << 1,
	UnorderedAccess = 1 << 2,
	IndirectBuffer = 1 << 3,
	KeepCPUAccessible = 1 << 4,
	Cubemap = 1 << 5
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

namespace drm
{
	class VertexBuffer
	{
	public:
		EFormat Format;
		EResourceUsage Usage;

		VertexBuffer(EFormat Format, EResourceUsage Usage)
			: Format(Format), Usage(Usage)
		{
		}
	};

	CLASS(VertexBuffer);

	class IndexBuffer
	{
	public:
		EFormat Format;
		EResourceUsage Usage;
		uint32 IndexStride;

		IndexBuffer(uint32 IndexStride, EFormat Format, EResourceUsage Usage)
			: IndexStride(IndexStride), Format(Format), Usage(Usage)
		{
		}
	};

	CLASS(IndexBuffer);

	class UniformBuffer
	{
	public:
		UniformBuffer(uint32 Size)
		{
			Data.resize(Size);
		}

		template<typename UniformType>
		void Set(const UniformType& UniformData)
		{
			check(sizeof(UniformType) == Data.size(), "Size mismatch.");
			Platform.Memcpy(Data.data(), &UniformData, sizeof(UniformType));
			Set();
		}

		const void* GetData() const { return Data.data(); }

	private:
		std::vector<std::byte> Data;
		// Allows implementations to do their own thing when data is set.
		virtual void Set() {}
	};

	CLASS(UniformBuffer);

	class StorageBuffer
	{
	public:
		const EResourceUsage Usage;

		StorageBuffer(EResourceUsage Usage)
			: Usage(Usage)
		{
		}
	};

	CLASS(StorageBuffer);

	class Image
	{
	public:
		EFormat Format;
		uint32 Width;
		uint32 Height;
		EResourceUsage Usage;
		EImageLayout Layout;

		Image(EFormat Format, EImageLayout Layout, uint32 Width, uint32 Height, EResourceUsage UsageFlags)
			: Format(Format), Layout(Layout), Width(Width), Height(Height), Usage(UsageFlags)
		{
		}

		bool IsColor() const;
		bool IsStencil() const;

		static bool IsDepthStencil(EFormat Format);
		bool IsDepthStencil() const;

		static bool IsDepth(EFormat Format);
		bool IsDepth() const;
	};

	CLASS(Image);

	class RenderTargetView
	{
	public:
		ImageRef Image;
		ELoadAction LoadAction;
		EStoreAction StoreAction;
		std::variant<ClearColorValue, ClearDepthStencilValue> ClearValue;
		EImageLayout FinalLayout;

		RenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout FinalLayout);
		RenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil, EImageLayout FinalLayout);
	};

	CLASS(RenderTargetView);
}
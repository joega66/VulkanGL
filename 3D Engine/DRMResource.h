#pragma once
#include <Platform/Platform.h>

enum class EImageFormat
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
	float DepthClear;
	uint32 StencilClear;
};

namespace drm
{
	class VertexBuffer
	{
	public:
		EImageFormat Format;
		EResourceUsage Usage;

		VertexBuffer(EImageFormat Format, EResourceUsage Usage)
			: Format(Format), Usage(Usage)
		{
		}
	};

	CLASS(VertexBuffer);

	class IndexBuffer
	{
	public:
		EImageFormat Format;
		EResourceUsage Usage;
		uint32 IndexStride;

		IndexBuffer(uint32 IndexStride, EImageFormat Format, EResourceUsage Usage)
			: IndexStride(IndexStride), Format(Format), Usage(Usage)
		{
		}
	};

	CLASS(IndexBuffer);

	class UniformBuffer
	{
	public:
		bool bDirty = false;

		template<typename UniformType>
		void Set(const UniformType& UniformData)
		{
			check(GetSize() == sizeof(UniformType), "Size mismatch.");
			Data = std::make_shared<UniformType>(UniformData);
			bDirty = true;
		}

		const void* GetData() { return Data.get(); }
		virtual uint32 GetSize() = 0;

	private:
		std::shared_ptr<void> Data;
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
		EImageFormat Format;
		uint32 Width;
		uint32 Height;
		EResourceUsage Usage;

		Image(EImageFormat Format, uint32 Width, uint32 Height, EResourceUsage UsageFlags)
			: Format(Format), Width(Width), Height(Height), Usage(UsageFlags)
		{
		}

		bool IsColor();
		bool IsDepthStencil();
		bool IsStencil();
		static bool IsDepth(EImageFormat Format);
		bool IsDepth();
	};

	CLASS(Image);

	class RenderTargetView
	{
	public:
		ImageRef Image;
		ELoadAction LoadAction;
		EStoreAction StoreAction;

		std::variant<std::array<float, 4>, ClearDepthStencilValue> ClearValue;

		RenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
		RenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil);

		bool operator==(const RenderTargetView& Other);
	};

	CLASS(RenderTargetView);
}
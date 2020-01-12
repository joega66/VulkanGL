#include "DRMResource.h"

namespace drm
{
	bool Image::IsColor() const
	{
		return !(IsDepth() || IsStencil() || Format == EFormat::UNDEFINED);
	}

	bool Image::IsStencil() const
	{
		static const std::unordered_set<EFormat> StencilFormats =
		{
			EFormat::S8_UINT
		};
		return IsDepthStencil() || StencilFormats.find(Format) != StencilFormats.end();
	}

	bool Image::IsDepthStencil(EFormat Format)
	{
		static const std::unordered_set<EFormat> DepthFormats =
		{
			EFormat::D32_SFLOAT_S8_UINT, EFormat::D24_UNORM_S8_UINT
		};
		return DepthFormats.find(Format) != DepthFormats.end();
	}

	bool Image::IsDepthStencil() const
	{
		return IsDepthStencil(Format);
	}

	bool Image::IsDepth(EFormat Format)
	{
		static const std::unordered_set<EFormat> DepthFormats =
		{
			EFormat::D16_UNORM, EFormat::D32_SFLOAT, EFormat::D32_SFLOAT_S8_UINT, EFormat::D24_UNORM_S8_UINT
		};
		return DepthFormats.find(Format) != DepthFormats.end();
	}

	uint32 Image::GetSize(EFormat Format)
	{
		static HashTable<EFormat, uint32> EngineFormatStrides =
		{
			ENTRY(EFormat::R8_UNORM, 1)
			ENTRY(EFormat::R8G8B8A8_UNORM, 4)
			ENTRY(EFormat::R32_SFLOAT, 4)
			ENTRY(EFormat::R32_SINT, 4)
			ENTRY(EFormat::R32_UINT, 4)
			ENTRY(EFormat::R32G32B32A32_SFLOAT, 16)
			ENTRY(EFormat::R32G32B32_SFLOAT, 12)
			ENTRY(EFormat::R32G32_SFLOAT, 8)
		};

		return EngineFormatStrides[Format];
	}

	bool Image::IsDepth() const
	{
		return IsDepth(Format);
	}

	uint32 Image::GetStrideInBytes() const
	{
		return GetSize(Format);
	}

	AttachmentView::AttachmentView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout FinalLayout)
		: Image(Image)
		, LoadAction(LoadAction)
		, StoreAction(StoreAction)
		, ClearValue(ClearValue)
		, FinalLayout(FinalLayout)
	{
	}

	AttachmentView::AttachmentView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil, EImageLayout FinalLayout)
		: Image(Image)
		, LoadAction(LoadAction)
		, StoreAction(StoreAction)
		, ClearValue(DepthStencil)
		, FinalLayout(FinalLayout)
	{
	}
}
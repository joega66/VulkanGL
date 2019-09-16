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

	bool Image::IsDepth() const
	{
		return IsDepth(Format);
	}

	RenderTargetView::RenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout FinalLayout)
		: Image(Image)
		, LoadAction(LoadAction)
		, StoreAction(StoreAction)
		, ClearValue(ClearValue)
		, FinalLayout(FinalLayout)
	{
	}

	RenderTargetView::RenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil, EImageLayout FinalLayout)
		: Image(Image)
		, LoadAction(LoadAction)
		, StoreAction(StoreAction)
		, ClearValue(DepthStencil)
		, FinalLayout(FinalLayout)
	{
	}
}
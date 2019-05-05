#include "DRMResource.h"

namespace drm
{
	bool Image::IsColor()
	{
		return !(IsDepth() || IsStencil() || IF_UNDEFINED);
	}

	bool Image::IsDepthStencil()
	{
		const std::unordered_set<EImageFormat> DepthFormats =
		{
			IF_D32_SFLOAT_S8_UINT, IF_D24_UNORM_S8_UINT
		};
		return DepthFormats.find(Format) != DepthFormats.end();
	}

	bool Image::IsStencil()
	{
		const std::unordered_set<EImageFormat> StencilFormats =
		{
			IF_S8_UINT
		};
		return IsDepthStencil() || StencilFormats.find(Format) != StencilFormats.end();
	}

	bool Image::IsDepth(EImageFormat Format)
	{
		const std::unordered_set<EImageFormat> DepthFormats =
		{
			IF_D16_UNORM, IF_D32_SFLOAT, IF_D32_SFLOAT_S8_UINT, IF_D24_UNORM_S8_UINT
		};
		return DepthFormats.find(Format) != DepthFormats.end();
	}

	bool Image::IsDepth()
	{
		return IsDepth(Format);
	}

	RenderTargetView::RenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
		: Image(Image)
		, LoadAction(LoadAction)
		, StoreAction(StoreAction)
		, ClearValue(ClearValue)
	{
	}

	RenderTargetView::RenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil)
		: Image(Image)
		, LoadAction(LoadAction)
		, StoreAction(StoreAction)
		, ClearValue(DepthStencil)
	{
	}
}
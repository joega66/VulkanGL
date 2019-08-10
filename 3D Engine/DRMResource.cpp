#include "DRMResource.h"

namespace drm
{
	bool Image::IsColor()
	{
		return !(IsDepth() || IsStencil() || Format == EImageFormat::UNDEFINED);
	}

	bool Image::IsStencil()
	{
		static const std::unordered_set<EImageFormat> StencilFormats =
		{
			EImageFormat::S8_UINT
		};
		return IsDepthStencil() || StencilFormats.find(Format) != StencilFormats.end();
	}

	bool Image::IsDepthStencil(EImageFormat Format)
	{
		static const std::unordered_set<EImageFormat> DepthFormats =
		{
			EImageFormat::D32_SFLOAT_S8_UINT, EImageFormat::D24_UNORM_S8_UINT
		};
		return DepthFormats.find(Format) != DepthFormats.end();
	}

	bool Image::IsDepthStencil()
	{
		return IsDepthStencil(Format);
	}

	bool Image::IsDepth(EImageFormat Format)
	{
		static const std::unordered_set<EImageFormat> DepthFormats =
		{
			EImageFormat::D16_UNORM, EImageFormat::D32_SFLOAT, EImageFormat::D32_SFLOAT_S8_UINT, EImageFormat::D24_UNORM_S8_UINT
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

	bool RenderTargetView::operator==(const RenderTargetView& Other)
	{
		// Don't compare clear value since it's not part of the render pass.
		return Image == Other.Image && LoadAction == Other.LoadAction && StoreAction == Other.StoreAction;
	}
}
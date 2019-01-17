#include "GLRenderResource.h"

bool GLImage::IsColor()
{
	return !(IsDepth() || IsStencil() || IF_UNDEFINED);
}

bool GLImage::IsDepthStencil()
{
	const std::unordered_set<EImageFormat> DepthFormats =
	{
		IF_D32_SFLOAT_S8_UINT, IF_D24_UNORM_S8_UINT
	};
	return DepthFormats.find(Format) != DepthFormats.end();
}

bool GLImage::IsStencil()
{
	const std::unordered_set<EImageFormat> StencilFormats =
	{
		IF_S8_UINT
	};
	return IsDepthStencil() || StencilFormats.find(Format) != StencilFormats.end();
}

bool GLImage::IsDepth(EImageFormat Format)
{
	const std::unordered_set<EImageFormat> DepthFormats =
	{
		IF_D16_UNORM, IF_D32_SFLOAT, IF_D32_SFLOAT_S8_UINT, IF_D24_UNORM_S8_UINT
	};
	return DepthFormats.find(Format) != DepthFormats.end();
}

bool GLImage::IsDepth()
{
	return IsDepth(Format);
}

GLRenderTargetView::GLRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
	: Image(Image)
	, LoadAction(LoadAction)
	, StoreAction(StoreAction)
	, ClearValue(ClearValue)
{
}

GLRenderTargetView::GLRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil)
	: Image(Image)
	, LoadAction(LoadAction)
	, StoreAction(StoreAction)
	, ClearValue(DepthStencil)
{
}

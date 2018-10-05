#pragma once
#include "GLRenderResource.h"

class GLImage : public GLRenderResource
{
public:
	EImageFormat Format;
	uint32 Width;
	uint32 Height;
	EResourceUsageFlags UsageFlags;

	GLImage(EImageFormat Format, uint32 Width, uint32 Height, EResourceUsageFlags UsageFlags)
		: Format(Format), Width(Width), Height(Height), UsageFlags(UsageFlags) {}

	bool IsColor();
	bool IsDepthStencil();
	bool IsStencil();
	static bool IsDepth(EImageFormat Format);
	bool IsDepth();
};

CLASS(GLImage);

enum EDepthStencilAccess : uint32
{
	DS_None,
	DS_DepthWrite,
	DS_StencilWrite,
	DS_DepthWriteStencilWrite,
	DS_DepthReadStencilWrite,
	DS_DepthWriteStencilRead,
	DS_DepthReadStencilRead,
};

enum class ELoadAction
{
	None,
	Clear,
	Load
};

enum class EStoreAction
{
	None,
	Store
};

class GLRenderTargetView : public GLRenderResource
{
public:
	GLImageRef Image;
	ELoadAction LoadAction;
	EStoreAction StoreAction;

	std::array<float, 4> ClearValue;
	float DepthClear;
	uint32 StencilClear;

	GLRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
	GLRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, float DepthClear, uint32 StencilClear);
};

CLASS(GLRenderTargetView);
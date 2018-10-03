#pragma once
#include "GLRenderResource.h"

enum EImageFormat
{
	IF_UNDEFINED,
	IF_R8_UINT,
	IF_R8_SINT,
	IF_R8G8_UINT,
	IF_R8G8_SINT,
	IF_R8_SRGB,
	IF_R8G8_SRGB,
	IF_R8G8B8_UINT,
	IF_R8G8B8_SINT,
	IF_R8G8B8_SRGB,
	IF_B8G8R8_UINT,
	IF_B8G8R8_SINT,
	IF_B8G8R8_SRGB,
	IF_R8G8B8A8_UINT,
	IF_R8G8B8A8_SINT,
	IF_R8G8B8A8_SRGB,
	IF_R8G8B8A8_UNORM,
	IF_B8G8R8A8_UINT,
	IF_B8G8R8A8_SINT,
	IF_B8G8R8A8_SRGB,
	IF_B8G8R8A8_UNORM,
	IF_R16_UINT,
	IF_R16_SINT,
	IF_R16_SFLOAT,
	IF_R16G16_UINT,
	IF_R16G16_SINT,
	IF_R16G16_SFLOAT,
	IF_R16G16B16_UINT,
	IF_R16G16B16_SINT,
	IF_R16G16B16_SFLOAT,
	IF_R16G16B16A16_UINT,
	IF_R16G16B16A16_SINT,
	IF_R16G16B16A16_SFLOAT,
	IF_R32_UINT,
	IF_R32_SINT,
	IF_R32_SFLOAT,
	IF_R32G32_UINT,
	IF_R32G32_SINT,
	IF_R32G32_SFLOAT,
	IF_R32G32B32_UINT,
	IF_R32G32B32_SINT,
	IF_R32G32B32_SFLOAT,
	IF_R32G32B32A32_UINT,
	IF_R32G32B32A32_SINT,
	IF_R32G32B32A32_SFLOAT,
	IF_D16_UNORM,
	IF_D32_SFLOAT,
	IF_S8_UINT,
	IF_D32_SFLOAT_S8_UINT,
	IF_D24_UNORM_S8_UINT,
};

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
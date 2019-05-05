#include "DRM.h"

DRMRef GDRM;

GLIndexBufferRef GLCreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void * Data)
{
	return GDRM->CreateIndexBuffer(Format, NumIndices, Usage, Data);
}

GLVertexBufferRef GLCreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void * Data)
{
	return GDRM->CreateVertexBuffer(Format, NumElements, Usage, Data);
}

GLStorageBufferRef GLCreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage)
{
	return GDRM->CreateStorageBuffer(Size, Data, Usage);
}

GLImageRef GLCreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data)
{
	return GDRM->CreateImage(Width, Height, Format, UsageFlags, Data);
}

GLImageRef GLCreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
{
	return GDRM->CreateCubemap(Width, Height, Format, UsageFlags | EResourceUsage::Cubemap, CubemapCreateInfo);
}

GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	return GDRM->CreateRenderTargetView(GLImage, LoadAction, StoreAction, ClearValue);
}

GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil)
{
	return GDRM->CreateRenderTargetView(GLImage, LoadAction, StoreAction, DepthStencil);
}

GLRenderTargetViewRef GLGetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	return GDRM->GetSurfaceView(LoadAction, StoreAction, ClearValue);
}

void* GLLockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset)
{
	return GDRM->LockBuffer(VertexBuffer, Size, Offset);
}

void GLUnlockBuffer(GLVertexBufferRef VertexBuffer)
{
	GDRM->UnlockBuffer(VertexBuffer);
}

void* GLLockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset)
{
	return GDRM->LockBuffer(IndexBuffer, Size, Offset);
}

void GLUnlockBuffer(GLIndexBufferRef IndexBuffer)
{
	return GDRM->UnlockBuffer(IndexBuffer);
}

void GLRebuildResolutionDependents()
{
	GDRM->RebuildResolutionDependents();
}

std::string GLGetDeviceName()
{
	return GDRM->GetDRMName();
}

namespace drm
{
	GLImageRef GetSurface()
	{
		return GDRM->GetSurface();
	}
}
#include "GL.h"

RenderCommandListRef GRenderCmdList;

GLIndexBufferRef GLCreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void * Data)
{
	return GRenderCmdList->CreateIndexBuffer(Format, NumIndices, Usage, Data);
}

GLVertexBufferRef GLCreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void * Data)
{
	return GRenderCmdList->CreateVertexBuffer(Format, NumElements, Usage, Data);
}

GLStorageBufferRef GLCreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage)
{
	return GRenderCmdList->CreateStorageBuffer(Size, Data, Usage);
}

GLImageRef GLCreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data)
{
	return GRenderCmdList->CreateImage(Width, Height, Format, UsageFlags, Data);
}

GLImageRef GLCreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
{
	return GRenderCmdList->CreateCubemap(Width, Height, Format, UsageFlags | EResourceUsage::Cubemap, CubemapCreateInfo);
}

GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	return GRenderCmdList->CreateRenderTargetView(GLImage, LoadAction, StoreAction, ClearValue);
}

GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil)
{
	return GRenderCmdList->CreateRenderTargetView(GLImage, LoadAction, StoreAction, DepthStencil);
}

GLRenderTargetViewRef GLGetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	return GRenderCmdList->GetSurfaceView(LoadAction, StoreAction, ClearValue);
}

void* GLLockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset)
{
	return GRenderCmdList->LockBuffer(VertexBuffer, Size, Offset);
}

void GLUnlockBuffer(GLVertexBufferRef VertexBuffer)
{
	GRenderCmdList->UnlockBuffer(VertexBuffer);
}

void* GLLockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset)
{
	return GRenderCmdList->LockBuffer(IndexBuffer, Size, Offset);
}

void GLUnlockBuffer(GLIndexBufferRef IndexBuffer)
{
	return GRenderCmdList->UnlockBuffer(IndexBuffer);
}

void GLRebuildResolutionDependents()
{
	GRenderCmdList->RebuildResolutionDependents();
}

std::string GLGetDeviceName()
{
	return GRenderCmdList->GetDeviceName();
}
#include "DRM.h"

DRMRef GDRM;

namespace drm
{
	void BeginFrame()
	{
		GDRM->BeginFrame();
	}

	void EndFrame(RenderCommandListRef CmdList)
	{
		GDRM->EndFrame(CmdList);
	}

	RenderCommandListRef CreateCommandList()
	{
		return GDRM->CreateCommandList();
	}

	IndexBufferRef CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void* Data)
	{
		return GDRM->CreateIndexBuffer(Format, NumIndices, Usage, Data);
	}

	VertexBufferRef CreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void* Data)
	{
		return GDRM->CreateVertexBuffer(Format, NumElements, Usage, Data);
	}

	StorageBufferRef CreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage)
	{
		return GDRM->CreateStorageBuffer(Size, Data, Usage);
	}

	ImageRef CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data)
	{
		return GDRM->CreateImage(Width, Height, Format, UsageFlags, Data);
	}

	ImageRef CreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
	{
		return GDRM->CreateCubemap(Width, Height, Format, UsageFlags | EResourceUsage::Cubemap, CubemapCreateInfo);
	}

	RenderTargetViewRef CreateRenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
	{
		return GDRM->CreateRenderTargetView(Image, LoadAction, StoreAction, ClearValue);
	}

	RenderTargetViewRef CreateRenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil)
	{
		return GDRM->CreateRenderTargetView(Image, LoadAction, StoreAction, DepthStencil);
	}

	ImageRef GetSurface()
	{
		return GDRM->GetSurface();
	}

	RenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
	{
		return GDRM->GetSurfaceView(LoadAction, StoreAction, ClearValue);
	}

	void* LockBuffer(VertexBufferRef VertexBuffer, uint32 Size, uint32 Offset)
	{
		return GDRM->LockBuffer(VertexBuffer, Size, Offset);
	}

	void UnlockBuffer(VertexBufferRef VertexBuffer)
	{
		GDRM->UnlockBuffer(VertexBuffer);
	}

	void* LockBuffer(IndexBufferRef IndexBuffer, uint32 Size, uint32 Offset)
	{
		return GDRM->LockBuffer(IndexBuffer, Size, Offset);
	}

	void UnlockBuffer(IndexBufferRef IndexBuffer)
	{
		return GDRM->UnlockBuffer(IndexBuffer);
	}

	void RebuildResolutionDependents()
	{
		GDRM->RebuildResolutionDependents();
	}

	std::string GetDeviceName()
	{
		return GDRM->GetDRMName();
	}
}
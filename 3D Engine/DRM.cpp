#include "DRM.h"
#include "DRMPrivate.h"

DRMRef GDRM;

namespace drm
{
	void BeginFrame()
	{
		GDRM->BeginFrame();
	}

	void EndFrame()
	{
		GDRM->EndFrame();
	}

	void SubmitCommands(RenderCommandListRef CmdList)
	{
		GDRM->SubmitCommands(CmdList);
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

	UniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate UniformUsage)
	{
		return GDRM->CreateUniformBuffer(Size, Data, UniformUsage);
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

	void* LockBuffer(VertexBufferRef VertexBuffer)
	{
		return GDRM->LockBuffer(VertexBuffer);
	}

	void UnlockBuffer(VertexBufferRef VertexBuffer)
	{
		GDRM->UnlockBuffer(VertexBuffer);
	}

	void* LockBuffer(IndexBufferRef IndexBuffer)
	{
		return GDRM->LockBuffer(IndexBuffer);
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

	ShaderResourceTable CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta)
	{
		return GDRM->CompileShader(Worker, Meta);
	}
}

void DRM::CacheShader(drm::ShaderRef Shader)
{
	GDRM->Shaders[Shader->Type] = Shader;
}

drm::ShaderRef DRM::FindShader(std::type_index Type)
{
	if (Contains(GDRM->Shaders, Type))
	{
		return GDRM->Shaders[Type];
	}
	else
	{
		return nullptr;
	}
}

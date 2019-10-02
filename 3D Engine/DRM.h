#pragma once
#include "RenderCommandList.h"

struct TextureCreateInfo
{
	int32 Width;
	int32 Height;
	uint8* Data;
};

struct CubemapCreateInfo
{
	// +X, -X, +Y, -Y, +Z, -Z
	std::array<TextureCreateInfo, 6> CubeFaces;
};

/** Direct Rendering Manager */
class DRM
{
public:
	virtual void Init() = 0;
	virtual void Release() = 0;

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	
	virtual void SubmitCommands(RenderCommandListRef CmdList) = 0;
	virtual RenderCommandListRef CreateCommandList() = 0;
	virtual drm::DescriptorSetRef CreateDescriptorSet() = 0;
	virtual drm::IndexBufferRef CreateIndexBuffer(EFormat Format, uint32 NumIndices, EBufferUsage Usage, const void* Data = nullptr) = 0;
	virtual drm::VertexBufferRef CreateVertexBuffer(EFormat Format, uint32 NumElements, EBufferUsage Usage, const void* Data = nullptr) = 0;
	virtual drm::UniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate Usage) = 0;
	virtual drm::StorageBufferRef CreateStorageBuffer(uint32 Size, const void* Data, EBufferUsage Usage) = 0;
	virtual drm::ImageRef CreateImage(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const uint8* Data = nullptr) = 0;
	virtual drm::ImageRef CreateCubemap(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo) = 0;
	virtual drm::RenderTargetViewRef CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout FinalLayout) = 0;
	virtual drm::RenderTargetViewRef CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil, EImageLayout FinalLayout) = 0;
	virtual drm::ImageRef GetSurface() = 0;
	virtual drm::RenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue) = 0;
	virtual void* LockBuffer(drm::VertexBufferRef VertexBuffer) = 0;
	virtual void UnlockBuffer(drm::VertexBufferRef VertexBuffer) = 0;
	virtual void* LockBuffer(drm::IndexBufferRef IndexBuffer) = 0;
	virtual void UnlockBuffer(drm::IndexBufferRef IndexBuffer) = 0;
	virtual void* LockBuffer(drm::StorageBufferRef StorageBuffer) = 0;
	virtual void UnlockBuffer(drm::StorageBufferRef StorageBuffer) = 0;
	virtual std::string GetDRMName() = 0;
	virtual ShaderResourceTable CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta) = 0;

private:
	template<typename ShaderType>
	friend class ShaderMapRef;

	HashTable<std::type_index, drm::ShaderRef> Shaders;

	static void CacheShader(drm::ShaderRef Shader);
	static drm::ShaderRef FindShader(std::type_index Type);
};

CLASS(DRM);

namespace drm
{
	void BeginFrame();
	void EndFrame();
	void SubmitCommands(RenderCommandListRef CmdList);
	RenderCommandListRef CreateCommandList();
	drm::DescriptorSetRef CreateDescriptorSet();
	IndexBufferRef CreateIndexBuffer(EFormat Format, uint32 NumIndices, EBufferUsage Usage, const void* Data = nullptr);
	VertexBufferRef CreateVertexBuffer(EFormat Format, uint32 NumElements, EBufferUsage Usage, const void* Data = nullptr);
	UniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate UniformUsage);
	StorageBufferRef CreateStorageBuffer(uint32 Size, const void* Data, EBufferUsage Usage = EBufferUsage::None);
	ImageRef CreateImage(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const uint8* Data = nullptr);
	ImageRef CreateCubemap(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo);
	RenderTargetViewRef CreateRenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout FinalLayout);
	RenderTargetViewRef CreateRenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil, EImageLayout FinalLayout);
	ImageRef GetSurface();
	RenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue);
	void* LockBuffer(VertexBufferRef VertexBuffer);
	void UnlockBuffer(VertexBufferRef VertexBuffer);
	void* LockBuffer(IndexBufferRef IndexBuffer);
	void UnlockBuffer(IndexBufferRef IndexBuffer);
	void* LockBuffer(drm::StorageBufferRef StorageBuffer);
	void UnlockBuffer(drm::StorageBufferRef StorageBuffer);
	std::string GetDeviceName();
	ShaderResourceTable CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta);
}

template<typename ShaderType>
class ShaderMapRef
{
public:
	ShaderMapRef()
	{
		std::type_index Type = std::type_index(typeid(ShaderType));

		if (auto CachedShader = std::static_pointer_cast<ShaderType>(DRM::FindShader(Type)); CachedShader)
		{
			Shader = CachedShader;
		}
		else
		{
			ShaderCompilerWorker Worker;
			ShaderType::SetEnvironmentVariables(Worker);
			const auto&[Filename, EntryPoint, Stage] = ShaderType::GetShaderInfo();
			const ShaderMetadata Meta(Filename, EntryPoint, Stage, Type);
			const ShaderResourceTable ResourceTable = drm::CompileShader(Worker, Meta);
			Shader = MakeRef<ShaderType>(ResourceTable);
			DRM::CacheShader(Shader);
		}
	}

	Ref<ShaderType> operator*()
	{
		return Shader;
	}

private:
	Ref<ShaderType> Shader;
};
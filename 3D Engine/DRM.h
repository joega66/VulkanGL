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
	virtual drm::IndexBufferRef CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void* Data = nullptr) = 0;
	virtual drm::VertexBufferRef CreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void* Data = nullptr) = 0;
	virtual drm::UniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate Usage) = 0;
	virtual drm::StorageBufferRef CreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage) = 0;
	virtual drm::ImageRef CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data = nullptr) = 0;
	virtual drm::ImageRef CreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo) = 0;
	virtual drm::RenderTargetViewRef CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) = 0;
	virtual drm::RenderTargetViewRef CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil) = 0;
	virtual drm::ImageRef GetSurface() = 0;
	virtual drm::RenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) = 0;
	virtual void* LockBuffer(drm::VertexBufferRef VertexBuffer, uint32 Size, uint32 Offset) = 0;
	virtual void UnlockBuffer(drm::VertexBufferRef VertexBuffer) = 0;
	virtual void* LockBuffer(drm::IndexBufferRef IndexBuffer, uint32 Size, uint32 Offset) = 0;
	virtual void UnlockBuffer(drm::IndexBufferRef IndexBuffer) = 0;
	virtual void RebuildResolutionDependents() = 0;
	virtual std::string GetDRMName() = 0;

	template<typename ShaderType>
	drm::ShaderRef CompileShader()
	{
		if (auto CachedShader = (
			FindShader(std::type_index(typeid(ShaderType)))); CachedShader)
		{
			return CachedShader;
		}
		else
		{
			ShaderCompilerWorker Worker;
			ShaderType::ModifyCompilationEnvironment(Worker);
			const auto&[Filename, EntryPoint, Stage] = ShaderType::GetBaseShaderInfo();
			ShaderMetadata Meta(Filename, EntryPoint, Stage);
			drm::ShaderRef Shader = CompileShader(Worker, Meta);
			CacheShader(std::type_index(typeid(ShaderType)), Shader);
			return Shader;
		}
	}

private:
	HashTable<std::type_index, drm::ShaderRef> Shaders;

	virtual drm::ShaderRef CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta) = 0;

	void CacheShader(std::type_index Type, drm::ShaderRef Shader)
	{
		Shaders[Type] = Shader;
	}

	drm::ShaderRef FindShader(std::type_index Type)
	{
		if (Contains(Shaders, Type))
		{
			return Shaders[Type];
		}
		else
		{
			return nullptr;
		}
	}
};

CLASS(DRM);

extern DRMRef GDRM;

namespace drm
{
	void BeginFrame();
	void EndFrame();
	void SubmitCommands(RenderCommandListRef CmdList);
	RenderCommandListRef CreateCommandList();
	IndexBufferRef CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void* Data = nullptr);
	VertexBufferRef CreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void* Data = nullptr);
	StorageBufferRef CreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage = EResourceUsage::None);
	ImageRef CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data = nullptr);
	ImageRef CreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo);
	RenderTargetViewRef CreateRenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
	RenderTargetViewRef CreateRenderTargetView(ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil);
	ImageRef GetSurface();
	RenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
	void* LockBuffer(VertexBufferRef VertexBuffer, uint32 Size, uint32 Offset);
	void UnlockBuffer(VertexBufferRef VertexBuffer);
	void* LockBuffer(IndexBufferRef IndexBuffer, uint32 Size, uint32 Offset);
	void UnlockBuffer(IndexBufferRef IndexBuffer);
	void RebuildResolutionDependents();
	std::string GetDeviceName();

	template<typename ShaderType>
	inline ShaderRef CreateShader()
	{
		return GDRM->CompileShader<ShaderType>();
	}

	template<typename UniformBufferType>
	inline UniformBufferRef CreateUniformBuffer(EUniformUpdate Usage = EUniformUpdate::Infrequent)
	{
		return GDRM->CreateUniformBuffer(sizeof(UniformBufferType), nullptr, Usage);
	}

	template<typename UniformBufferType>
	inline UniformBufferRef CreateUniformBuffer(const UniformBufferType& Data, EUniformUpdate Usage = EUniformUpdate::Infrequent)
	{
		return GDRM->CreateUniformBuffer(sizeof(UniformBufferType), &Data, Usage);
	}
}

template<typename ShaderType>
class ShaderMapRef
{
public:
	ShaderMapRef()
	{
		Shader = GDRM->CompileShader<ShaderType>();
	}

private:
	std::shared_ptr<ShaderType> Shader;
};
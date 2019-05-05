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
	virtual void EndFrame(RenderCommandListRef CmdList) = 0;

	virtual RenderCommandListRef CreateCommandList() = 0;
	virtual GLIndexBufferRef CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void* Data = nullptr) = 0;
	virtual GLVertexBufferRef CreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void* Data = nullptr) = 0;
	virtual GLUniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate Usage) = 0;
	virtual GLStorageBufferRef CreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage) = 0;
	virtual GLImageRef CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data = nullptr) = 0;
	virtual GLImageRef CreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo) = 0;
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) = 0;
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil) = 0;
	virtual GLImageRef GetSurface() = 0;
	virtual GLRenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) = 0;
	virtual void* LockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset) = 0;
	virtual void UnlockBuffer(GLVertexBufferRef VertexBuffer) = 0;
	virtual void* LockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset) = 0;
	virtual void UnlockBuffer(GLIndexBufferRef IndexBuffer) = 0;
	virtual void RebuildResolutionDependents() = 0;
	virtual std::string GetDRMName() = 0;
};

CLASS(DRM);

extern DRMRef GDRM;

GLIndexBufferRef GLCreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void* Data = nullptr);
GLVertexBufferRef GLCreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void* Data = nullptr);
GLStorageBufferRef GLCreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage = EResourceUsage::None);
GLImageRef GLCreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data = nullptr);
GLImageRef GLCreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo);
GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil);

namespace drm
{
	GLImageRef GetSurface();
}

GLRenderTargetViewRef GLGetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
void* GLLockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset);
void GLUnlockBuffer(GLVertexBufferRef VertexBuffer);
void* GLLockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset);
void GLUnlockBuffer(GLIndexBufferRef IndexBuffer);
void GLRebuildResolutionDependents();
std::string GLGetDeviceName();

template<typename ShaderType>
GLShaderRef GLCreateShader()
{
	if (GLShaderRef Shader = GShaderCompiler->FindShader(std::type_index(typeid(ShaderType))); Shader)
	{
		return Shader;
	}
	else
	{
		const auto& [Filename, EntryPoint, Stage] = ShaderType::GetBaseShaderInfo();
		return GShaderCompiler->CompileShader<ShaderType>(Filename, EntryPoint, Stage);
	}
}

template<typename UniformBufferType>
inline GLUniformBufferRef GLCreateUniformBuffer(EUniformUpdate Usage = EUniformUpdate::Infrequent)
{
	return GDRM->CreateUniformBuffer(sizeof(UniformBufferType), nullptr, Usage);
}

template<typename UniformBufferType>
inline GLUniformBufferRef GLCreateUniformBuffer(const UniformBufferType& Data, EUniformUpdate Usage = EUniformUpdate::Infrequent)
{
	return GDRM->CreateUniformBuffer(sizeof(UniformBufferType), &Data, Usage);
}
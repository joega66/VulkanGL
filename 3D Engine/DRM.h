#pragma once
#include "DRMCommandList.h"

#define UNIFORM_STRUCT(StructName, Members)		\
struct StructName								\
{												\
	Members										\
}; static_assert(sizeof(StructName) % 16 == 0);	\

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
	virtual ~DRM() {}

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void SubmitCommands(drm::CommandListRef CmdList) = 0;
	virtual drm::CommandListRef CreateCommandList() = 0;
	virtual drm::DescriptorSetRef CreateDescriptorSet() = 0;
	virtual drm::BufferRef CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data = nullptr) = 0;
	virtual drm::ImageRef CreateImage(uint32 Width, uint32 Height, uint32 Depth, EFormat Format, EImageUsage UsageFlags, const uint8* Data = nullptr) = 0;
	virtual drm::ImageRef CreateCubemap(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo) = 0;
	virtual drm::ImageRef GetSurface() = 0;
	virtual void* LockBuffer(drm::BufferRef Buffer) = 0;
	virtual void UnlockBuffer(drm::BufferRef Buffer) = 0;
	virtual std::string GetDRMName() = 0;
	virtual ShaderCompilationInfo CompileShader(const ShaderCompilerWorker& Worker, const ShaderMetadata& Meta) = 0;
	virtual void RecompileShaders() = 0;

protected:
	HashTable<std::type_index, drm::ShaderRef> Shaders;

private:
	template<typename ShaderType>
	friend class ShaderMapRef;

	static void CacheShader(drm::ShaderRef Shader);
	static drm::ShaderRef FindShader(std::type_index Type);
};

CLASS(DRM);

namespace drm
{
	void BeginFrame();
	void EndFrame();
	void SubmitCommands(drm::CommandListRef CmdList);
	drm::CommandListRef CreateCommandList();
	drm::DescriptorSetRef CreateDescriptorSet();
	drm::BufferRef CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data = nullptr);
	ImageRef CreateImage(uint32 Width, uint32 Height, uint32 Depth, EFormat Format, EImageUsage UsageFlags, const uint8* Data = nullptr);
	ImageRef CreateCubemap(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo);
	ImageRef GetSurface();
	void* LockBuffer(drm::BufferRef Buffer);
	void UnlockBuffer(drm::BufferRef Buffer);
	std::string GetDeviceName();
	ShaderCompilationInfo CompileShader(const ShaderCompilerWorker& Worker, const ShaderMetadata& Meta);
	void RecompileShaders();
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
			const ShaderCompilationInfo CompilationInfo = drm::CompileShader(Worker, Meta);
			Shader = MakeRef<ShaderType>(CompilationInfo);
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
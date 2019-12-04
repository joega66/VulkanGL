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

	drm::DescriptorSetRef CreateDescriptorSet()
	{
		return GDRM->CreateDescriptorSet();
	}

	drm::BufferRef CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data)
	{
		return GDRM->CreateBuffer(Usage, Size, Data);
	}

	ImageRef CreateImage(uint32 Width, uint32 Height, uint32 Depth, EFormat Format, EImageUsage UsageFlags, const uint8* Data)
	{
		return GDRM->CreateImage(Width, Height, Depth, Format, UsageFlags, Data);
	}

	ImageRef CreateCubemap(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
	{
		return GDRM->CreateCubemap(Width, Height, Format, UsageFlags | EImageUsage::Cubemap, CubemapCreateInfo);
	}

	ImageRef GetSurface()
	{
		return GDRM->GetSurface();
	}

	void* LockBuffer(drm::BufferRef Buffer)
	{
		return GDRM->LockBuffer(Buffer);
	}

	void UnlockBuffer(drm::BufferRef Buffer)
	{
		GDRM->UnlockBuffer(Buffer);
	}

	std::string GetDeviceName()
	{
		return GDRM->GetDRMName();
	}

	ShaderCompilationInfo CompileShader(const ShaderCompilerWorker& Worker, const ShaderMetadata& Meta)
	{
		return GDRM->CompileShader(Worker, Meta);
	}

	void RecompileShaders()
	{
		GDRM->RecompileShaders();
	}
}

void DRM::CacheShader(drm::ShaderRef Shader)
{
	GDRM->Shaders[Shader->CompilationInfo.Type] = Shader;
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
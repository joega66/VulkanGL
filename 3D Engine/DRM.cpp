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

	void SubmitCommands(drm::CommandListRef CmdList)
	{
		GDRM->SubmitCommands(CmdList);
	}

	drm::CommandListRef CreateCommandList()
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
}
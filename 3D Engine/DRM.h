#pragma once
#include "DRMCommandList.h"

#define UNIFORM_STRUCT(StructName, Members)		\
struct StructName								\
{												\
	Members										\
}; static_assert(sizeof(StructName) % 16 == 0);	\

/** Direct Rendering Manager */
class DRM
{
public:
	virtual ~DRM() {}

	virtual void EndFrame() = 0;
	virtual void SubmitCommands(drm::CommandListRef CmdList) = 0;
	virtual drm::CommandListRef CreateCommandList() = 0;
	virtual drm::DescriptorSetRef CreateDescriptorSet() = 0;
	virtual drm::BufferRef CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data = nullptr) = 0;
	virtual drm::ImageRef CreateImage(uint32 Width, uint32 Height, uint32 Depth, EFormat Format, EImageUsage UsageFlags) = 0;
	virtual void* LockBuffer(drm::BufferRef Buffer) = 0;
	virtual void UnlockBuffer(drm::BufferRef Buffer) = 0;
	virtual drm::RenderPassRef CreateRenderPass(const RenderPassInitializer& RPInit) = 0;
};

namespace drm
{
	/** Display engine interface. */
	class Surface
	{
	public:
		virtual uint32 AcquireNextImage(DRM& Device) = 0;
		virtual void Present(DRM& Device, uint32 ImageIndex, drm::CommandListRef CmdList) = 0;
		virtual void Resize(DRM& Device, uint32 ScreenWidth, uint32 ScreenHeight) = 0;
		virtual drm::ImageRef GetImage(uint32 ImageIndex) = 0;
	};
};
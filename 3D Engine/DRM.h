#pragma once
#include "DRMCommandList.h"

#define UNIFORM_STRUCT(StructName, Members)		\
struct StructName								\
{												\
	Members										\
}; static_assert(sizeof(StructName) % 16 == 0);	\

/** Direct Rendering Manager Device */
class DRMDevice
{
public:
	virtual ~DRMDevice() {}

	/** Needs to be called at the end of each frame to reset per-frame resources. */
	virtual void EndFrame() = 0;

	/** Submit a command list to the GPU. */
	virtual void SubmitCommands(drm::CommandListRef CmdList) = 0;

	/** Create a command list. */
	virtual drm::CommandListRef CreateCommandList() = 0;

	/** Create a descriptor set for listing the resources referenced in shaders. */
	virtual drm::DescriptorSetRef CreateDescriptorSet() = 0;

	/** Create a buffer resource. */
	virtual drm::BufferRef CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data = nullptr) = 0;

	/** Create an image resource. */
	virtual drm::ImageRef CreateImage(
		uint32 Width,
		uint32 Height,
		uint32 Depth,
		EFormat Format,
		EImageUsage UsageFlags,
		EImageLayout InitialLayout = EImageLayout::Undefined
	) = 0;

	/** Map a buffer into host memory. */
	virtual void* LockBuffer(drm::BufferRef Buffer) = 0;

	/** Unmap a buffer from host memory. */
	virtual void UnlockBuffer(drm::BufferRef Buffer) = 0;

	/** Create a render pass resource. */
	virtual drm::RenderPassRef CreateRenderPass(const RenderPassDesc& RPDesc) = 0;
};

namespace drm
{
	/** Compositor interface. Useful for creating a swapchain or letting an SDK control the display, e.g. OpenXR. */
	class Surface
	{
	public:
		virtual uint32 AcquireNextImage(DRMDevice& Device) = 0;
		virtual void Present(DRMDevice& Device, uint32 ImageIndex, drm::CommandListRef CmdList) = 0;
		virtual void Resize(DRMDevice& Device, uint32 ScreenWidth, uint32 ScreenHeight) = 0;
		virtual drm::ImageRef GetImage(uint32 ImageIndex) = 0;
	};
}
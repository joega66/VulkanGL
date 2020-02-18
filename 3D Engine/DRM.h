#pragma once
#include "DRMDefinitions.h"

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
	virtual void SubmitCommands(drm::CommandList& CmdList) = 0;

	/** Create a command list. */
	virtual drm::CommandList CreateCommandList(EQueue Queue) = 0;

	/** Create a descriptor template. */
	virtual drm::DescriptorTemplateRef CreateDescriptorTemplate(uint32 NumEntries, const DescriptorTemplateEntry* Entries) = 0;

	/** Create a buffer resource. */
	virtual drm::BufferRef CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data = nullptr) = 0;

	/** Create an image resource. */
	virtual drm::Image CreateImage(
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
	virtual drm::RenderPass CreateRenderPass(const RenderPassDesc& RPDesc) = 0;
};

namespace drm
{
	/** Compositor interface. Useful for creating a swapchain or letting an SDK control the display, e.g. OpenXR. */
	class Surface
	{
	public:
		virtual uint32 AcquireNextImage(DRMDevice& Device) = 0;
		virtual void Present(DRMDevice& Device, uint32 ImageIndex, drm::CommandList& CmdList) = 0;
		virtual void Resize(DRMDevice& Device, uint32 ScreenWidth, uint32 ScreenHeight) = 0;
		virtual const drm::Image& GetImage(uint32 ImageIndex) = 0;
	};

	void UploadImageData(DRMDevice& Device, const void* SrcPixels, const drm::Image& DstImage);
}

/** Descriptor set template helpers. */

template<typename DescriptorSetType>
class DescriptorTemplate
{
private:
	drm::DescriptorTemplateRef DescriptorUpdateTemplate;

public:
	DescriptorTemplate(DRMDevice& Device)
	{
		DescriptorUpdateTemplate = Device.CreateDescriptorTemplate(DescriptorSetType::GetEntries().size(), DescriptorSetType::GetEntries().data());
	}

	inline drm::DescriptorSetRef CreateDescriptorSet() 
	{ 
		return DescriptorUpdateTemplate->CreateDescriptorSet();
	}

	inline void UpdateDescriptorSet(drm::DescriptorSetRef DescriptorSet, DescriptorSetType& DescriptorWrite) 
	{ 
		DescriptorUpdateTemplate->UpdateDescriptorSet(DescriptorSet, &DescriptorWrite);
	}

	inline operator drm::DescriptorTemplateRef() { return DescriptorUpdateTemplate; }
};

template<typename DescriptorSetType>
class DescriptorSet : public DescriptorSetType
{
private:
	drm::DescriptorTemplateRef DescriptorTemplate;
	drm::DescriptorSetRef _DescriptorSet;

public:
	DescriptorSet(DRMDevice& Device)
	{
		DescriptorTemplate = Device.CreateDescriptorTemplate(DescriptorSetType::GetEntries().size(), DescriptorSetType::GetEntries().data());
		_DescriptorSet = DescriptorTemplate->CreateDescriptorSet();
	}

	inline void Update() 
	{ 
		DescriptorTemplate->UpdateDescriptorSet(_DescriptorSet, this);
	}

	inline operator drm::DescriptorSetRef() const { return _DescriptorSet; }
};
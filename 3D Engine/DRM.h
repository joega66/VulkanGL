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

	/** Create a graphics pipeline. */
	virtual drm::Pipeline CreatePipeline(const PipelineStateDesc& PSODesc) = 0;

	/** Create a compute pipeline. */
	virtual drm::Pipeline CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc) = 0;

	/** Create a descriptor template. */
	virtual drm::DescriptorSetLayout CreateDescriptorSetLayout(uint32 NumBindings, const DescriptorBinding* Bindings) = 0;

	/** Create a buffer resource. */
	virtual drm::Buffer CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data = nullptr) = 0;

	/** Create an image resource. */
	virtual drm::Image CreateImage(
		uint32 Width,
		uint32 Height,
		uint32 Depth,
		EFormat Format,
		EImageUsage UsageFlags,
		EImageLayout InitialLayout = EImageLayout::Undefined
	) = 0;

	/** Create a sampler resource. */
	virtual const drm::Sampler* CreateSampler(const SamplerDesc& SamplerDesc) = 0;

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

/** Descriptor set layout helpers. */

template<typename DescriptorSetType>
class DescriptorSetLayout
{
private:
	drm::DescriptorSetLayout _DescriptorSetLayout;

public:
	DescriptorSetLayout(DRMDevice& Device)
	{
		_DescriptorSetLayout = Device.CreateDescriptorSetLayout(DescriptorSetType::GetBindings().size(), DescriptorSetType::GetBindings().data());
	}

	inline drm::DescriptorSet CreateDescriptorSet() 
	{ 
		return _DescriptorSetLayout.CreateDescriptorSet();
	}

	inline void UpdateDescriptorSet(const drm::DescriptorSet& DescriptorSet, DescriptorSetType& DescriptorWrite) 
	{ 
		_DescriptorSetLayout.UpdateDescriptorSet(DescriptorSet, &DescriptorWrite);
	}

	inline operator const drm::DescriptorSetLayout&() { return _DescriptorSetLayout; }
};

template<typename DescriptorSetType>
class DescriptorSet : public DescriptorSetType
{
private:
	drm::DescriptorSetLayout DescriptorSetLayout;
	drm::DescriptorSet _DescriptorSet;

public:
	DescriptorSet(DRMDevice& Device)
	{
		DescriptorSetLayout = Device.CreateDescriptorSetLayout(DescriptorSetType::GetBindings().size(), DescriptorSetType::GetBindings().data());
		_DescriptorSet = DescriptorSetLayout.CreateDescriptorSet();
	}

	inline void Update() 
	{ 
		DescriptorSetLayout.UpdateDescriptorSet(_DescriptorSet, this);
	}

	inline operator const drm::DescriptorSet*() const { return &_DescriptorSet; }
};
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

	virtual void EndFrame() = 0;

	virtual void SubmitCommands(drm::CommandList& CmdList) = 0;

	virtual drm::CommandList CreateCommandList(EQueue Queue) = 0;

	virtual std::shared_ptr<drm::Pipeline> CreatePipeline(const PipelineStateDesc& PSODesc) = 0;

	virtual std::shared_ptr<drm::Pipeline> CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc) = 0;

	virtual drm::DescriptorSetLayout CreateDescriptorSetLayout(std::size_t NumBindings, const DescriptorBinding* Bindings) = 0;

	virtual drm::Buffer CreateBuffer(EBufferUsage Usage, uint64 Size, const void* Data = nullptr) = 0;

	virtual drm::Image CreateImage(
		uint32 Width,
		uint32 Height,
		uint32 Depth,
		EFormat Format,
		EImageUsage UsageFlags,
		uint32 MipLevels = 1
	) = 0;

	virtual drm::ImageView CreateImageView(
		const drm::Image& Image,
		uint32 BaseMipLevel,
		uint32 LevelCount,
		uint32 BaseArrayLayer,
		uint32 LayerCount
	) = 0;

	virtual drm::Sampler CreateSampler(const SamplerDesc& SamplerDesc) = 0;

	virtual drm::RenderPass CreateRenderPass(const RenderPassDesc& RPDesc) = 0;

	virtual drm::TextureID CreateTextureID(const drm::ImageView& ImageView) = 0;

	virtual drm::BindlessResources& GetTextures() = 0;
	
	virtual drm::BindlessResources& GetSamplers() = 0;
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

	inline drm::DescriptorSet CreateDescriptorSet(DRMDevice& Device) 
	{ 
		return _DescriptorSetLayout.CreateDescriptorSet(Device);
	}

	inline void UpdateDescriptorSet(DRMDevice& Device, const drm::DescriptorSet& DescriptorSet, DescriptorSetType& DescriptorWrite)
	{ 
		_DescriptorSetLayout.UpdateDescriptorSet(Device, DescriptorSet, &DescriptorWrite);
	}

	inline operator VkDescriptorSetLayout() const { return _DescriptorSetLayout; }
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
		_DescriptorSet = DescriptorSetLayout.CreateDescriptorSet(Device);
	}

	inline void Update(DRMDevice& Device)
	{ 
		DescriptorSetLayout.UpdateDescriptorSet(Device, _DescriptorSet, this);
	}

	inline VkDescriptorSetLayout GetLayout() const { return DescriptorSetLayout; }
	inline operator VkDescriptorSet() const { return _DescriptorSet; }
};
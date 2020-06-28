#pragma once
#include "DRMDefinitions.h"

#define UNIFORM_STRUCT(StructName, Members)		\
struct StructName								\
{												\
	Members										\
}; static_assert(sizeof(StructName) % 16 == 0);	\

namespace drm
{
	/** Direct Rendering Manager Device */
	class Device
	{
	public:
		virtual ~Device() {}

		virtual void EndFrame() = 0;

		virtual void SubmitCommands(drm::CommandList& cmdList) = 0;

		virtual drm::CommandList CreateCommandList(EQueue queue) = 0;

		virtual drm::Pipeline CreatePipeline(const PipelineStateDesc& psoDesc) = 0;

		virtual drm::Pipeline CreatePipeline(const ComputePipelineDesc& computePipelineDesc) = 0;

		virtual drm::DescriptorSetLayout CreateDescriptorSetLayout(
			std::size_t numBindings, 
			const DescriptorBinding* bindings
		) = 0;

		virtual drm::Buffer CreateBuffer(
			EBufferUsage usage, 
			uint64 size, 
			const void* data = nullptr
		) = 0;

		virtual drm::Image CreateImage(
			uint32 width,
			uint32 height,
			uint32 depth,
			EFormat format,
			EImageUsage usageFlags,
			uint32 mipLevels = 1
		) = 0;

		virtual drm::ImageView CreateImageView(
			const drm::Image& image,
			uint32 baseMipLevel,
			uint32 levelCount,
			uint32 baseArrayLayer,
			uint32 layerCount
		) = 0;

		virtual drm::Sampler CreateSampler(const SamplerDesc& samplerDesc) = 0;

		virtual drm::RenderPass CreateRenderPass(const RenderPassDesc& rpDesc) = 0;

		virtual drm::TextureID CreateTextureID(const drm::ImageView& imageView) = 0;

		virtual drm::ImageID CreateImageID(const drm::ImageView& imageView) = 0;

		virtual drm::BindlessResources& GetTextures() = 0;

		virtual drm::BindlessResources& GetSamplers() = 0;

		virtual drm::BindlessResources& GetImages() = 0;
	};

	/** Compositor interface. Useful for creating a swapchain or letting an SDK control the display, e.g. OpenXR. */
	class Surface
	{
	public:
		virtual uint32 AcquireNextImage(drm::Device& device) = 0;
		virtual void Present(drm::Device& device, uint32 imageIndex, drm::CommandList& cmdList) = 0;
		virtual void Resize(drm::Device& device, uint32 screenWidth, uint32 screenHeight) = 0;
		virtual const drm::Image& GetImage(uint32 imageIndex) = 0;
		virtual const std::vector<drm::Image>& GetImages() = 0;
	};

	void UploadImageData(drm::Device& device, const void* srcPixels, const drm::Image& dstImage);
}

/** Descriptor set layout helpers. */

template<typename DescriptorSetType>
class DescriptorSetLayout
{
private:
	drm::DescriptorSetLayout _DescriptorSetLayout;

public:
	DescriptorSetLayout(drm::Device& device)
	{
		_DescriptorSetLayout = device.CreateDescriptorSetLayout(DescriptorSetType::GetBindings().size(), DescriptorSetType::GetBindings().data());
	}

	inline drm::DescriptorSet CreateDescriptorSet(drm::Device& device)
	{ 
		return _DescriptorSetLayout.CreateDescriptorSet(device);
	}

	inline void UpdateDescriptorSet(drm::Device& device, const drm::DescriptorSet& descriptorSet, DescriptorSetType& descriptorWrite)
	{ 
		_DescriptorSetLayout.UpdateDescriptorSet(device, descriptorSet, &descriptorWrite);
	}

	inline operator VkDescriptorSetLayout() const { return _DescriptorSetLayout; }
};

template<typename DescriptorSetType>
class DescriptorSet : public DescriptorSetType
{
private:
	drm::DescriptorSetLayout _DescriptorSetLayout;
	drm::DescriptorSet _DescriptorSet;

public:
	DescriptorSet(drm::Device& device)
	{
		_DescriptorSetLayout = device.CreateDescriptorSetLayout(DescriptorSetType::GetBindings().size(), DescriptorSetType::GetBindings().data());
		_DescriptorSet = _DescriptorSetLayout.CreateDescriptorSet(device);
	}

	inline void Update(drm::Device& device)
	{ 
		_DescriptorSetLayout.UpdateDescriptorSet(device, _DescriptorSet, this);
	}

	inline VkDescriptorSetLayout GetLayout() const { return _DescriptorSetLayout; }
	inline operator VkDescriptorSet() const { return _DescriptorSet; }
};
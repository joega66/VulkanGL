#pragma once
#include "GPUDefinitions.h"

#define UNIFORM_STRUCT(StructName, Members)		\
struct StructName								\
{												\
	Members										\
}; static_assert(sizeof(StructName) % 16 == 0);	\

namespace gpu
{
	class Device
	{
	public:
		virtual ~Device() {}

		virtual void EndFrame() = 0;

		virtual void SubmitCommands(gpu::CommandList& cmdList) = 0;

		virtual gpu::CommandList CreateCommandList(EQueue queue) = 0;

		virtual gpu::Pipeline CreatePipeline(const PipelineStateDesc& psoDesc) = 0;

		virtual gpu::Pipeline CreatePipeline(const ComputePipelineDesc& computePipelineDesc) = 0;

		virtual gpu::DescriptorSetLayout CreateDescriptorSetLayout(
			std::size_t numBindings, 
			const DescriptorBinding* bindings
		) = 0;

		virtual gpu::Buffer CreateBuffer(
			EBufferUsage usage, 
			uint64 size, 
			const void* data = nullptr
		) = 0;

		virtual gpu::Image CreateImage(
			uint32 width,
			uint32 height,
			uint32 depth,
			EFormat format,
			EImageUsage usageFlags,
			uint32 mipLevels = 1
		) = 0;

		virtual gpu::ImageView CreateImageView(
			const gpu::Image& image,
			uint32 baseMipLevel,
			uint32 levelCount,
			uint32 baseArrayLayer,
			uint32 layerCount
		) = 0;

		virtual gpu::Sampler CreateSampler(const SamplerDesc& samplerDesc) = 0;

		virtual gpu::RenderPass CreateRenderPass(const RenderPassDesc& rpDesc) = 0;

		virtual gpu::TextureID CreateTextureID(const gpu::ImageView& imageView) = 0;

		virtual gpu::ImageID CreateImageID(const gpu::ImageView& imageView) = 0;

		virtual gpu::BindlessResources& GetTextures() = 0;

		virtual gpu::BindlessResources& GetSamplers() = 0;

		virtual gpu::BindlessResources& GetImages() = 0;
	};

	/** Compositor interface. Useful for creating a swapchain or letting an SDK control the display, e.g. OpenXR. */
	class Surface
	{
	public:
		virtual uint32 AcquireNextImage(gpu::Device& device) = 0;
		virtual void Present(gpu::Device& device, uint32 imageIndex, gpu::CommandList& cmdList) = 0;
		virtual void Resize(gpu::Device& device, uint32 screenWidth, uint32 screenHeight) = 0;
		virtual const gpu::Image& GetImage(uint32 imageIndex) = 0;
		virtual const std::vector<gpu::Image>& GetImages() = 0;
	};

	void UploadImageData(gpu::Device& device, const void* srcPixels, const gpu::Image& dstImage);
}

/** Descriptor set layout helpers. */

template<typename DescriptorSetType>
class DescriptorSetLayout
{
private:
	gpu::DescriptorSetLayout _DescriptorSetLayout;

public:
	DescriptorSetLayout(gpu::Device& device)
	{
		_DescriptorSetLayout = device.CreateDescriptorSetLayout(DescriptorSetType::GetBindings().size(), DescriptorSetType::GetBindings().data());
	}

	inline gpu::DescriptorSet CreateDescriptorSet(gpu::Device& device)
	{ 
		return _DescriptorSetLayout.CreateDescriptorSet(device);
	}

	inline void UpdateDescriptorSet(gpu::Device& device, const gpu::DescriptorSet& descriptorSet, DescriptorSetType& descriptorWrite)
	{ 
		_DescriptorSetLayout.UpdateDescriptorSet(device, descriptorSet, &descriptorWrite);
	}

	inline operator VkDescriptorSetLayout() const { return _DescriptorSetLayout; }
};

template<typename DescriptorSetType>
class DescriptorSet : public DescriptorSetType
{
private:
	gpu::DescriptorSetLayout _DescriptorSetLayout;
	gpu::DescriptorSet _DescriptorSet;

public:
	DescriptorSet(gpu::Device& device)
	{
		_DescriptorSetLayout = device.CreateDescriptorSetLayout(DescriptorSetType::GetBindings().size(), DescriptorSetType::GetBindings().data());
		_DescriptorSet = _DescriptorSetLayout.CreateDescriptorSet(device);
	}

	inline void Update(gpu::Device& device)
	{ 
		_DescriptorSetLayout.UpdateDescriptorSet(device, _DescriptorSet, this);
	}

	inline VkDescriptorSetLayout GetLayout() const { return _DescriptorSetLayout; }
	inline operator VkDescriptorSet() const { return _DescriptorSet; }
};
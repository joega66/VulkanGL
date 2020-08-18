#pragma once
#include "GPUDefinitions.h"

#define UNIFORM_STRUCT(StructName, Members)		\
struct StructName								\
{												\
	Members										\
}; static_assert(sizeof(StructName) % 16 == 0);	\

struct DeviceDesc
{
	void* windowHandle;
	bool enableValidationLayers;
};

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

		template<typename DescriptorSetType>
		gpu::DescriptorSet CreateDescriptorSet()
		{
			gpu::DescriptorSetLayout& layout = DescriptorSetType::GetLayout(*this);
			return layout.CreateDescriptorSet(*this);
		}

		template<typename DescriptorSetType>
		gpu::DescriptorSet CreateDescriptorSet(DescriptorSetType& descriptors)
		{
			gpu::DescriptorSetLayout& layout = DescriptorSetType::GetLayout(*this);
			gpu::DescriptorSet set = layout.CreateDescriptorSet(*this);
			layout.UpdateDescriptorSet(*this, set, &descriptors);
			return set;
		}

		template<typename DescriptorSetType>
		void UpdateDescriptorSet(const gpu::DescriptorSet& set, DescriptorSetType& descriptors)
		{
			gpu::DescriptorSetLayout& layout = DescriptorSetType::GetLayout(*this);
			layout.UpdateDescriptorSet(*this, set, &descriptors);
		}
	};

	/** Compositor interface. Useful for creating a swapchain or letting an SDK control the display, e.g. OpenXR. */
	class Surface
	{
	public:
		virtual uint32 AcquireNextImage() = 0;
		virtual void Present(uint32 imageIndex, gpu::CommandList& cmdList) = 0;
		virtual void Resize(uint32 screenWidth, uint32 screenHeight, EImageUsage imageUsage) = 0;
		virtual const gpu::Image& GetImage(uint32 imageIndex) = 0;
		virtual const std::vector<gpu::Image>& GetImages() = 0;
	};

	void UploadImageData(gpu::Device& device, const void* srcPixels, const gpu::Image& dstImage);
}
#pragma once
#include "VulkanBindlessDescriptors.h"
#include <GPU/GPUShader.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

class VulkanDevice;

namespace gpu
{
	class ImageView
	{
	public:
		ImageView(const ImageView&) = delete;
		ImageView& operator=(const ImageView&) = delete;

		ImageView() = default;
		ImageView(VulkanDevice& device, VkImageView imageView, EImageUsage usage, EFormat format);
		ImageView(ImageView&& other);
		ImageView& operator=(ImageView&& other);
		~ImageView();

		inline VkImageView GetHandle() const { return _ImageView; }
		inline EFormat GetFormat() const { return _Format; }
		inline const TextureID& GetTextureID() const { return _TextureID; }
		inline const ImageID& GetImageID() const { return _ImageID; }

	private:
		VkDevice _Device = nullptr;
		VkImageView _ImageView = nullptr;
		TextureID _TextureID = {};
		ImageID _ImageID = {};
		EFormat _Format;
	};

	class Image : public gpu::ImagePrivate
	{
	public:
		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		Image() = default;
		Image(
			VulkanDevice& device
			, VmaAllocator allocator
			, VmaAllocation allocation
			, const VmaAllocationInfo& allocationInfo
			, VkImage image
			, EFormat format
			, uint32 width
			, uint32 height
			, uint32 depth
			, EImageUsage usage
			, uint32 mipLevels);
		Image(Image&& other);
		Image& operator=(Image&& other);
		~Image();

		inline operator VkImage() const { return _Image; }
		inline operator const ImageView& () const { return _ImageView; }
		inline VkImage GetHandle() const { return _Image; }
		inline const ImageView& GetImageView() const { return _ImageView; }
		inline const TextureID& GetTextureID() const { return _ImageView.GetTextureID(); }
		inline const ImageID& GetImageID() const { return _ImageView.GetImageID(); }
		VkFormat GetVulkanFormat() const;
		VkImageAspectFlags GetVulkanAspect() const;

		static VkFormat FindSupportedDepthFormat(VulkanDevice& device, EFormat format);
		static VkFormat GetVulkanFormat(EFormat format);
		static EFormat GetEngineFormat(VkFormat format);
		static VkImageLayout GetLayout(EImageLayout layout);
		static bool IsDepthLayout(VkImageLayout layout);
		static VkFilter GetVulkanFilter(EFilter filter);

	private:
		VmaAllocator _Allocator;
		VmaAllocation _Allocation;
		VmaAllocationInfo _AllocationInfo;
		VkImage _Image = nullptr;
		ImageView _ImageView = {};
	};

	class Sampler
	{
	public:
		Sampler() = default;
		Sampler(VulkanDevice& device, const SamplerDesc& samplerDesc);

		inline VkSampler GetHandle() const { return _Sampler; }
		inline const SamplerID& GetSamplerID() const { return _SamplerID; }

	private:
		VkSampler _Sampler = nullptr;
		SamplerID _SamplerID = {};
	};

	class SampledImage
	{
	public:
		SampledImage() = default;
		SampledImage(const ImageView& imageView, const Sampler& sampler);

		static EDescriptorType GetDescriptorType() { return EDescriptorType::SampledImage; }

	private:
		VkDescriptorImageInfo _DescriptorImageInfo = { nullptr };
	};

	class StorageImage
	{
	public:
		StorageImage() = default;
		StorageImage(const Image& image);
		StorageImage(const ImageView& imageView);

		static EDescriptorType GetDescriptorType() { return EDescriptorType::StorageImage; }

	private:
		VkDescriptorImageInfo _DescriptorImageInfo = { nullptr };
	};
};
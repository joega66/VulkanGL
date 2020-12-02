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

		inline operator const VkImageView&() const { return _ImageView; }
		inline EFormat GetFormat() const { return _Format; }
		TextureID GetTextureID(const gpu::Sampler& sampler);
		inline ImageID GetImageID() const { return _ImageID; }

	private:
		VulkanDevice*	_Device = nullptr;
		EFormat			_Format = EFormat::UNDEFINED;
		VkImageView		_ImageView = nullptr;
		ImageID			_ImageID = {};
		std::unordered_map<VkSampler, TextureID> _TextureIDs = {};
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
		inline const ImageView& GetImageView() const { return _ImageView; }
		inline TextureID GetTextureID(const Sampler& sampler) { return _ImageView.GetTextureID(sampler); }
		inline ImageID GetImageID() const { return _ImageView.GetImageID(); }
		VkFormat GetVulkanFormat() const;
		VkImageAspectFlags GetVulkanAspect() const;

		static VkFormat FindSupportedDepthFormat(VulkanDevice& device, EFormat format);
		static VkFormat GetVulkanFormat(EFormat format);
		static EFormat GetEngineFormat(VkFormat format);
		static VkImageLayout GetLayout(EImageLayout layout);
		static bool IsDepthLayout(VkImageLayout layout);
		static VkFilter GetVulkanFilter(EFilter filter);

	private:
		VmaAllocator		_Allocator;
		VmaAllocation		_Allocation;
		VmaAllocationInfo	_AllocationInfo;
		VkImage				_Image = nullptr;
		ImageView			_ImageView = {};
	};

	class Sampler
	{
	public:
		Sampler() = default;
		Sampler(VulkanDevice& device, const SamplerDesc& samplerDesc);

		inline operator const VkSampler&() const { return _Sampler; }

	private:
		VkSampler _Sampler = nullptr;
	};

	class SampledImage
	{
	public:
		SampledImage() = default;
		SampledImage(const ImageView& imageView, const Sampler& sampler);

		static VkDescriptorType GetDescriptorType() { return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; }

	private:
		VkDescriptorImageInfo _DescriptorImageInfo = { nullptr };
	};

	class StorageImage
	{
	public:
		StorageImage() = default;
		StorageImage(const Image& image);
		StorageImage(const ImageView& imageView);

		static VkDescriptorType GetDescriptorType() { return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; }

	private:
		VkDescriptorImageInfo _DescriptorImageInfo = { nullptr };
	};
};
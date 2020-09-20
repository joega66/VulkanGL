#pragma once
#include "VulkanBindlessResources.h"
#include <GPU/GPUShader.h>
#include "vk_mem_alloc.h"

class VulkanDevice;

class VulkanImageView
{
public:
	VulkanImageView(const VulkanImageView&) = delete;
	VulkanImageView& operator=(const VulkanImageView&) = delete;

	VulkanImageView() = default;
	VulkanImageView(VulkanDevice& device, VkImageView imageView, EFormat format);
	VulkanImageView(VulkanImageView&& other);
	VulkanImageView& operator=(VulkanImageView&& other);
	~VulkanImageView();

	inline VkImageView GetHandle() const { return _ImageView; }
	inline EFormat GetFormat() const { return _Format; }

private:
	VkDevice _Device = VK_NULL_HANDLE;
	VkImageView _ImageView = nullptr;
	EFormat _Format;
};

class VulkanImage : public gpu::ImagePrivate
{
public:
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	VulkanImage() = default;
	VulkanImage(
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
	VulkanImage(VulkanImage&& other);
	VulkanImage& operator=(VulkanImage&& other);
	~VulkanImage();

	inline void* GetNativeHandle() { return _Image; }
	inline operator VkImage() const { return _Image; }
	inline operator const VulkanImageView& () const { return _ImageView; }
	inline VkImage GetHandle() const { return _Image; }
	inline const VulkanImageView& GetImageView() const { return _ImageView; }
	inline const VulkanTextureID& GetTextureID() const { return _TextureID; }
	inline const VulkanImageID& GetImageID() const { return _ImageID; }
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
	VulkanImageView _ImageView = {};
	VulkanTextureID _TextureID = {};
	VulkanImageID _ImageID = {};
};

class VulkanSampler
{
public:
	VulkanSampler() = default;
	VulkanSampler(VulkanDevice& device, const SamplerDesc& samplerDesc);

	inline VkSampler GetHandle() const { return _Sampler; }
	inline const VulkanSamplerID& GetSamplerID() const { return _SamplerID; }

private:
	VkSampler _Sampler = nullptr;
	VulkanSamplerID _SamplerID = {};
};

namespace gpu
{
	class SampledImage
	{
	public:
		SampledImage() = default;
		SampledImage(const VulkanImageView& imageView, const VulkanSampler& sampler);

		static EDescriptorType GetDescriptorType() { return EDescriptorType::SampledImage; }

	private:
		VkDescriptorImageInfo _DescriptorImageInfo = { nullptr };
	};

	class StorageImage
	{
	public:
		StorageImage() = default;
		StorageImage(const VulkanImage& image);
		StorageImage(const VulkanImageView& imageView);

		static EDescriptorType GetDescriptorType() { return EDescriptorType::StorageImage; }

	private:
		VkDescriptorImageInfo _DescriptorImageInfo = { nullptr };
	};
}
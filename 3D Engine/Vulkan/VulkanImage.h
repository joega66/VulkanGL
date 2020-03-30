#pragma once
#include <DRMResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanImageView
{
public:
	VulkanImageView(const VulkanImageView&) = delete;
	VulkanImageView& operator=(const VulkanImageView&) = delete;

	VulkanImageView() = default;
	VulkanImageView(VulkanDevice& Device, VkImageView ImageView, EFormat Format);
	VulkanImageView(VulkanImageView&& Other);
	VulkanImageView& operator=(VulkanImageView&& Other);
	~VulkanImageView();

	inline VkImageView GetHandle() const { return ImageView; }
	inline EFormat GetFormat() const { return Format; }

private:
	VkDevice Device = VK_NULL_HANDLE;
	VkImageView ImageView = nullptr;
	EFormat Format;
};

class VulkanImage : public drm::ImagePrivate
{
public:
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	VulkanImage() = default;
	VulkanImage(VulkanDevice& Device
		, VkImage Image
		, VkDeviceMemory Memory
		, EFormat Format
		, uint32 Width
		, uint32 Height
		, uint32 Depth
		, EImageUsage UsageFlags
		, uint32 MipLevels
	);
	VulkanImage(VulkanImage&& Other);
	VulkanImage& operator=(VulkanImage&& Other);
	~VulkanImage();

	inline void* GetNativeHandle() { return Image; }
	inline operator VkImage() const { return Image; }
	inline VkImage GetHandle() const { return Image; }
	inline const VulkanImageView& GetImageView() const { return ImageView; }
	VkFormat GetVulkanFormat() const;
	VkImageAspectFlags GetVulkanAspect() const;
	
	static VkFormat FindSupportedDepthFormat(VulkanDevice& Device, EFormat Format);
	static VkFormat GetVulkanFormat(EFormat Format);
	static EFormat GetEngineFormat(VkFormat Format);
	static VkImageLayout GetVulkanLayout(EImageLayout Layout);
	static bool IsDepthLayout(VkImageLayout Layout);
	static VkFilter GetVulkanFilter(EFilter Filter);

private:
	VkDevice Device = VK_NULL_HANDLE;
	VkImage Image = nullptr;
	VulkanImageView ImageView = {};
	VkDeviceMemory Memory = nullptr;
};

class VulkanSampler
{
public:
	VulkanSampler() = default;
	VulkanSampler(VulkanDevice& Device, const SamplerDesc& SamplerDesc);

	inline VkSampler GetHandle() const { return Sampler; }

private:
	VkSampler Sampler = nullptr;
};

class VulkanDescriptorImageInfo
{
public:
	VulkanDescriptorImageInfo() = default;
	VulkanDescriptorImageInfo(const VulkanImageView& ImageView);
	VulkanDescriptorImageInfo(const VulkanImage& Image);
	VulkanDescriptorImageInfo(const VulkanImageView& ImageView, const VulkanSampler& Sampler);
	VulkanDescriptorImageInfo(const VulkanImage& Image, const VulkanSampler& Sampler);

	void SetImage(const VulkanImage& Image);

	bool operator==(const VulkanImage& Image);
	bool operator!=(const VulkanImage& Image);

private:
	VkDescriptorImageInfo DescriptorImageInfo = { nullptr };
};
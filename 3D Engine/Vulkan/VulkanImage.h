#pragma once
#include <DRMResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanImageView
{
public:
	VulkanImageView() = default;
	VulkanImageView(VulkanDevice& Device, VkImageView ImageView, EFormat Format);
	VulkanImageView(VulkanImageView&& Other);
	VulkanImageView& operator=(VulkanImageView&& Other);
	~VulkanImageView();

	inline VkImageView GetNativeHandle() const { return ImageView; }
	inline EFormat GetFormat() const { return Format; }

private:
	VulkanDevice* Device = nullptr;
	VkImageView ImageView = nullptr;
	EFormat Format;
};

class VulkanImage : public drm::ImagePrivate
{
public:
	VkImage Image = nullptr;
	VulkanImageView ImageView;
	VkDeviceMemory Memory = nullptr;

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

	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	inline void* GetNativeHandle() { return Image; }

	operator VkImage();

	static VkFormat FindSupportedDepthFormat(VulkanDevice& Device, EFormat Format);

	static VkFormat GetVulkanFormat(EFormat Format);

	static EFormat GetEngineFormat(VkFormat Format);

	static VkImageLayout GetVulkanLayout(EImageLayout Layout);

	static bool IsDepthLayout(VkImageLayout Layout);

	static VkFilter GetVulkanFilter(EFilter Filter);

	[[nodiscard]] static VkSampler CreateSampler(VulkanDevice& Device, const struct SamplerDesc& SamplerDesc);

	VkFormat GetVulkanFormat() const;

	VkImageAspectFlags GetVulkanAspect() const;

private:
	VulkanDevice* Device = nullptr;
};

class VulkanSampler
{
public:
	VulkanSampler() = default;
	VulkanSampler(VkSampler Sampler);
	inline VkSampler GetHandle() const { return Sampler; }

private:
	VkSampler Sampler = nullptr;
};

class VulkanDescriptorImageInfo
{
public:
	VulkanDescriptorImageInfo() = default;
	VulkanDescriptorImageInfo(const VulkanImageView& ImageView, const VulkanSampler* Sampler = nullptr);
	VulkanDescriptorImageInfo(const VulkanImage& Image, const VulkanSampler* Sampler = nullptr);

	void SetImage(const VulkanImage& Image);

	/** Whether the image view is here. */
	bool operator==(const VulkanImage& Image);
	bool operator!=(const VulkanImage& Image);

private:
	VkDescriptorImageInfo DescriptorImageInfo = { nullptr };
};
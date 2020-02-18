#pragma once
#include <DRMResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanImage : public drm::__Image
{
public:
	VkImage Image = VK_NULL_HANDLE;
	VkImageView ImageView = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;

	VulkanImage() = default;
	VulkanImage(VulkanDevice& Device
		, VkImage Image
		, VkDeviceMemory Memory
		, EFormat Format
		, uint32 Width
		, uint32 Height
		, uint32 Depth
		, EImageUsage UsageFlags);
	VulkanImage(VulkanImage&& Other);
	VulkanImage& operator=(VulkanImage&& Other);
	~VulkanImage();

	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	inline uint64 GetNativeHandle() { return Image; }

	operator VkImage();

	static VkFormat FindSupportedDepthFormat(VulkanDevice& Device, EFormat Format);

	static VkFormat GetVulkanFormat(EFormat Format);

	static EFormat GetEngineFormat(VkFormat Format);

	static VkImageLayout GetVulkanLayout(EImageLayout Layout);

	static bool IsDepthLayout(VkImageLayout Layout);

	static VkFilter GetVulkanFilter(EFilter Filter);

	[[nodiscard]] static VkSampler CreateSampler(VulkanDevice& Device, const struct SamplerState& SamplerState);

	VkFormat GetVulkanFormat() const;

	VkImageAspectFlags GetVulkanAspect() const;

private:
	VulkanDevice* Device;
};

CLASS(VulkanImage);
#pragma once
#include "../DRMResource.h"
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanImage : public drm::Image
{
public:
	VkImage Image;
	VkImageView ImageView;
	VkDeviceMemory Memory;

	VulkanImage(VulkanDevice& Device 
		, VkImage Image
		, VkDeviceMemory Memory
		, EFormat Format
		, EImageLayout Layout
		, uint32 Width
		, uint32 Height
		, uint32 Depth
		, EImageUsage UsageFlags);
	
	~VulkanImage();

	operator VkImage();

	static VkFormat FindSupportedDepthFormat(VulkanDevice& Device, EFormat Format);

	static VkFormat GetVulkanFormat(EFormat Format);

	static EFormat GetEngineFormat(VkFormat Format);

	static VkImageLayout GetVulkanLayout(EImageLayout Layout);

	static bool IsDepthLayout(VkImageLayout Layout);

	[[nodiscard]] static VkSampler CreateSampler(VulkanDevice& Device, const struct SamplerState& SamplerState);

	VkFormat GetVulkanFormat() const;

	VkImageLayout GetVulkanLayout() const;

	VkImageAspectFlags GetVulkanAspect() const;

private:
	VulkanDevice& Device;
};

CLASS(VulkanImage);
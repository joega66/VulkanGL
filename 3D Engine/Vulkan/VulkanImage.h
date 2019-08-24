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
	VkPipelineStageFlags Stage;

	VulkanImage(VulkanDevice& Device 
		, VkImage Image
		, VkDeviceMemory Memory
		, EImageFormat Format
		, EImageLayout Layout
		, uint32 Width
		, uint32 Height
		, EResourceUsage UsageFlags
		, VkPipelineStageFlags Stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	
	~VulkanImage();

	operator VkImage();

	static VkFormat FindSupportedDepthFormat(VulkanDevice& Device, EImageFormat Format);

	static VkFormat GetVulkanFormat(EImageFormat Format);

	static EImageFormat GetEngineFormat(VkFormat Format);

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
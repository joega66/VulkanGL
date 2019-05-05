#pragma once
#include "../DRMResource.h"
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanImage : public GLImage
{
public:
	VkImage Image;
	VkImageView ImageView;
	VkDeviceMemory Memory;
	VkImageLayout Layout;
	VkPipelineStageFlags Stage;

	VulkanImage(VulkanDevice& Device 
		, VkImage Image
		, VkDeviceMemory Memory
		, VkImageLayout Layout
		, EImageFormat Format
		, uint32 Width
		, uint32 Height
		, EResourceUsage UsageFlags
		, VkPipelineStageFlags Stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	~VulkanImage();

	operator VkImage();

	static VkFormat GetVulkanFormat(EImageFormat Format);
	static EImageFormat GetEngineFormat(VkFormat Format);
	static bool IsDepthLayout(VkImageLayout Layout);
	static VkSampler CreateSampler(VulkanDevice& Device, const struct SamplerState& SamplerState);

	bool IsInDepthLayout();
	VkFormat GetVulkanFormat() const;
	VkImageAspectFlags GetVulkanAspect();

private:
	VulkanDevice& Device;
};

CLASS(VulkanImage);
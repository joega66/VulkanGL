#include "VulkanImage.h"
#include "VulkanGL.h"
#include "VulkanCommands.h"

VulkanImage::VulkanImage(VulkanDevice & Device, VkImage Image, VkDeviceMemory Memory, VkImageLayout Layout, EImageFormat Format, uint32 Width, uint32 Height, EResourceUsageFlags UsageFlags, VkPipelineStageFlags Stage)
	: Device(Device)
	, Image(Image)
	, Memory(Memory)
	, Layout(Layout)
	, Stage(Stage)
	, GLImage(Format, Width, Height, UsageFlags)
{
}

void VulkanImage::ReleaseGL()
{
	vkDestroyImageView(Device, ImageView, nullptr);
	vkDestroyImage(Device, Image, nullptr);
	vkFreeMemory(Device, Memory, nullptr);
}

VulkanImage::operator VkImage() { return Image; }

bool VulkanImage::IsDepthLayout(VkImageLayout Layout)
{
	std::unordered_set<VkImageLayout> VulkanDepthLayouts =
	{
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
		, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
	};

	return Contains(VulkanDepthLayouts, Layout);
}

bool VulkanImage::IsInDepthLayout()
{
	return IsDepthLayout(Layout);
}

VkFormat VulkanImage::GetVulkanFormat() const
{
	return GetValue(VulkanFormat, Format);
}

VkImageAspectFlags VulkanImage::GetVulkanAspect()
{
	VkFlags Flags = 0;
	if (IsDepthStencil())
	{
		Flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else if (IsDepth())
	{
		Flags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (IsStencil())
	{
		Flags = VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		Flags = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	return Flags;
}

static VkFormat FindSupportedFormat(VulkanDevice& Device, const std::vector<VkFormat>& Candidates, VkImageTiling Tiling, VkFormatFeatureFlags Features)
{
	for (VkFormat Format : Candidates)
	{
		VkFormatProperties Props;
		vkGetPhysicalDeviceFormatProperties(Device, Format, &Props);

		if (Tiling == VK_IMAGE_TILING_LINEAR && (Props.linearTilingFeatures & Features) == Features)
		{
			return Format;
		}
		else if (Tiling == VK_IMAGE_TILING_OPTIMAL && (Props.optimalTilingFeatures & Features) == Features)
		{
			return Format;
		}
	}

	fail("Failed to find supported format.");
}

VkFormat VulkanGL::FindSupportedDepthFormat(EImageFormat Format)
{
	const std::vector<VkFormat> Candidates =
	{
		GetValue(VulkanFormat, Format),
		VK_FORMAT_D32_SFLOAT, 
		VK_FORMAT_D32_SFLOAT_S8_UINT, 
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};

	return FindSupportedFormat(Device, Candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
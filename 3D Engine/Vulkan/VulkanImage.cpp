#include "VulkanImage.h"
#include "VulkanGL.h"
#include "VulkanCommands.h"

const HashTable<EImageFormat, VkFormat> VulkanFormat =
{
	ENTRY(IF_UNDEFINED, VK_FORMAT_UNDEFINED)
	ENTRY(IF_R8G8_UINT, VK_FORMAT_R8_UINT)
	ENTRY(IF_R8G8_SINT, VK_FORMAT_R8_SINT)
	ENTRY(IF_R8_SRGB, VK_FORMAT_R8_SRGB)
	ENTRY(IF_R8G8_UINT, VK_FORMAT_R8G8_UINT)
	ENTRY(IF_R8G8_SINT, VK_FORMAT_R8G8_SINT)
	ENTRY(IF_R8G8_SRGB, VK_FORMAT_R8G8_SRGB)
	ENTRY(IF_R8G8B8_UINT, VK_FORMAT_R8G8B8_UINT)
	ENTRY(IF_R8G8B8_SINT, VK_FORMAT_R8G8B8_SINT)
	ENTRY(IF_R8G8B8_SRGB, VK_FORMAT_R8G8B8_SRGB)
	ENTRY(IF_B8G8R8_UINT, VK_FORMAT_B8G8R8_UINT)
	ENTRY(IF_B8G8R8_SINT, VK_FORMAT_B8G8R8_SINT)
	ENTRY(IF_B8G8R8_SRGB, VK_FORMAT_B8G8R8_SRGB)
	ENTRY(IF_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT)
	ENTRY(IF_R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_SINT)
	ENTRY(IF_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB)
	ENTRY(IF_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM)
	ENTRY(IF_B8G8R8A8_UINT, VK_FORMAT_B8G8R8A8_UINT)
	ENTRY(IF_B8G8R8A8_SINT, VK_FORMAT_B8G8R8A8_SINT)
	ENTRY(IF_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB)
	ENTRY(IF_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM)
	ENTRY(IF_R16_UINT, VK_FORMAT_R16_UINT)
	ENTRY(IF_R16_SINT, VK_FORMAT_R16_SINT)
	ENTRY(IF_R16_SFLOAT, VK_FORMAT_R16_SFLOAT)
	ENTRY(IF_R16G16_UINT, VK_FORMAT_R16G16_UINT)
	ENTRY(IF_R16G16_SINT, VK_FORMAT_R16G16_SINT)
	ENTRY(IF_R16G16_SFLOAT, VK_FORMAT_R16G16_SFLOAT)
	ENTRY(IF_R16G16B16_UINT, VK_FORMAT_R16G16B16_UINT)
	ENTRY(IF_R16G16B16_SINT, VK_FORMAT_R16G16B16_SINT)
	ENTRY(IF_R16G16B16_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT)
	ENTRY(IF_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT)
	ENTRY(IF_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT)
	ENTRY(IF_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT)
	ENTRY(IF_R32_UINT, VK_FORMAT_R32_UINT)
	ENTRY(IF_R32_SINT, VK_FORMAT_R32_SINT)
	ENTRY(IF_R32_SFLOAT, VK_FORMAT_R32_SFLOAT)
	ENTRY(IF_R32G32_UINT, VK_FORMAT_R32G32_UINT)
	ENTRY(IF_R32G32_SINT, VK_FORMAT_R32G32_SINT)
	ENTRY(IF_R32G32_SFLOAT, VK_FORMAT_R32G32_SFLOAT)
	ENTRY(IF_R32G32B32_UINT, VK_FORMAT_R32G32B32_UINT)
	ENTRY(IF_R32G32B32_SINT, VK_FORMAT_R32G32B32_SINT)
	ENTRY(IF_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT)
	ENTRY(IF_R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_UINT)
	ENTRY(IF_R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_SINT)
	ENTRY(IF_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT)
	ENTRY(IF_D16_UNORM, VK_FORMAT_D16_UNORM)
	ENTRY(IF_D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT)
	ENTRY(IF_D32_SFLOAT, VK_FORMAT_D32_SFLOAT)
	ENTRY(IF_S8_UINT, VK_FORMAT_S8_UINT)
	ENTRY(IF_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT)
	ENTRY(IF_BC2_UNORM_BLOCK, VK_FORMAT_BC2_UNORM_BLOCK)
};

VulkanImage::VulkanImage(VulkanDevice& Device, VkImage Image, VkDeviceMemory Memory, VkImageLayout Layout, EImageFormat Format, uint32 Width, uint32 Height, EResourceUsage UsageFlags, VkPipelineStageFlags Stage)
	: Device(Device)
	, Image(Image)
	, Memory(Memory)
	, Layout(Layout)
	, Stage(Stage)
	, GLImage(Format, Width, Height, UsageFlags)
{
	VkImageViewCreateInfo ViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	ViewInfo.image = Image;
	ViewInfo.viewType = Any(Usage & EResourceUsage::Cubemap) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	ViewInfo.format = GetVulkanFormat();
	ViewInfo.subresourceRange.aspectMask = GetVulkanAspect();
	ViewInfo.subresourceRange.baseMipLevel = 0;
	ViewInfo.subresourceRange.levelCount = 1;
	ViewInfo.subresourceRange.baseArrayLayer = 0;
	ViewInfo.subresourceRange.layerCount = Any(Usage & EResourceUsage::Cubemap) ? 6 : 1;
	ViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	vulkan(vkCreateImageView(Device, &ViewInfo, nullptr, &ImageView));
}

void VulkanImage::ReleaseGL()
{
	vkDestroyImageView(Device, ImageView, nullptr);
	vkDestroyImage(Device, Image, nullptr);
	vkFreeMemory(Device, Memory, nullptr);
}

VulkanImage::operator VkImage() { return Image; }

VkFormat VulkanImage::GetVulkanFormat(EImageFormat Format)
{
	return GetValue(VulkanFormat, Format);
}

EImageFormat VulkanImage::GetEngineFormat(VkFormat Format)
{
	return GetKey(VulkanFormat, Format);
}

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

VkFormat VulkanCommandList::FindSupportedDepthFormat(EImageFormat Format)
{
	const std::vector<VkFormat> Candidates =
	{
		VulkanImage::GetVulkanFormat(Format),
		VK_FORMAT_D32_SFLOAT, 
		VK_FORMAT_D32_SFLOAT_S8_UINT, 
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};

	return FindSupportedFormat(Device, Candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
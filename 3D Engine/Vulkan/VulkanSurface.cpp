#include "VulkanSurface.h"
#include "VulkanDRM.h"
#include "VulkanDevice.h"
#include <Platform/Platform.h>
#include <Engine/Screen.h>

VulkanSurface::VulkanSurface(VulkanDevice& Device)
	: Device(Device)
{
}

void VulkanSurface::Init()
{
	SwapchainSupportDetails SwapchainSupport = {};
	SwapchainSupport.QuerySwapchainSupport(Device.PhysicalDevice, Device.Surface);

	VkSurfaceFormatKHR SurfaceFormat = ChooseSwapSurfaceFormat(SwapchainSupport.Formats);
	VkPresentModeKHR PresentMode = ChooseSwapPresentMode(SwapchainSupport.PresentModes);

	VkExtent2D Extent = ChooseSwapExtent(SwapchainSupport.Capabilities);

	uint32 ImageCount = SwapchainSupport.Capabilities.minImageCount + 1;

	if (SwapchainSupport.Capabilities.maxImageCount > 0 && ImageCount > SwapchainSupport.Capabilities.maxImageCount)
	{
		ImageCount = SwapchainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR SwapchainInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	SwapchainInfo.surface = Device.Surface;
	SwapchainInfo.minImageCount = ImageCount;
	SwapchainInfo.imageFormat = SurfaceFormat.format;
	SwapchainInfo.imageColorSpace = SurfaceFormat.colorSpace;
	SwapchainInfo.imageExtent = Extent;
	SwapchainInfo.imageArrayLayers = 1;
	SwapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const uint32 QueueFamilyIndices[] = { static_cast<uint32>(Device.Queues.GetGraphicsIndex()), static_cast<uint32>(Device.Queues.GetPresentIndex()) };

	if (QueueFamilyIndices[0] != QueueFamilyIndices[1])
	{
		SwapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		SwapchainInfo.queueFamilyIndexCount = 2;
		SwapchainInfo.pQueueFamilyIndices = QueueFamilyIndices;
	}
	else
	{
		SwapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	SwapchainInfo.preTransform = SwapchainSupport.Capabilities.currentTransform;
	SwapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapchainInfo.presentMode = PresentMode;
	SwapchainInfo.clipped = VK_TRUE;

	vulkan(vkCreateSwapchainKHR(Device, &SwapchainInfo, nullptr, &Swapchain));

	vkGetSwapchainImagesKHR(Device, Swapchain, &ImageCount, nullptr);

	std::vector<VkImage> VkImages(ImageCount);

	vkGetSwapchainImagesKHR(Device, Swapchain, &ImageCount, VkImages.data());

	Images.resize(ImageCount);

	for (uint32 i = 0; i < ImageCount; i++)
	{
		Images[i] = MakeRef<VulkanImage>(Device
			, VkImages[i]
			, VkDeviceMemory()
			, VulkanImage::GetEngineFormat(SurfaceFormat.format)
			, EImageLayout::Undefined
			, Extent.width
			, Extent.height
			, 1
			, EImageUsage::RenderTargetable);
	}
}

void VulkanSurface::Free()
{
	//vkDestroySwapchainKHR(Device, Swapchain, nullptr);
}

VkSurfaceFormatKHR VulkanSurface::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats)
{
	if (AvailableFormats.size() == 1 && AvailableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& AvailableFormat : AvailableFormats)
	{
		if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return AvailableFormat;
		}
	}

	return AvailableFormats[0];
}

VkPresentModeKHR VulkanSurface::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvailablePresentModes)
{
	VkPresentModeKHR BestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& AvailablePresentMode : AvailablePresentModes)
	{
		if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return AvailablePresentMode;
		}
		else if (AvailablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			BestMode = AvailablePresentMode;
		}
	}

	return BestMode;
}

VkExtent2D VulkanSurface::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& Capabilities)
{
	if (Capabilities.currentExtent.width != std::numeric_limits<uint32>::max())
	{
		return Capabilities.currentExtent;
	}
	else
	{
		VkExtent2D ActualExtent = 
		{
			(uint32)gScreen.GetWidth(),
			(uint32)gScreen.GetHeight()
		};

		ActualExtent.width = std::max(Capabilities.minImageExtent.width, std::min(Capabilities.maxImageExtent.width, ActualExtent.width));
		ActualExtent.height = std::max(Capabilities.minImageExtent.height, std::min(Capabilities.maxImageExtent.height, ActualExtent.height));

		return ActualExtent;
	}
}

void SwapchainSupportDetails::QuerySwapchainSupport(VkPhysicalDevice Device, VkSurfaceKHR Surface)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, Surface, &Capabilities);

	uint32 FormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, nullptr);

	if (FormatCount != 0)
	{
		Formats.resize(FormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, Formats.data());
	}

	uint32 PresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, nullptr);

	if (PresentModeCount != 0)
	{
		PresentModes.resize(PresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, PresentModes.data());
	}
}

#include "VulkanSurface.h"
#include "VulkanGL.h"
#include "VulkanDevice.h"
#include "Platform/Platform.h"
#include <algorithm>

VulkanSurface::VulkanSurface(VulkanDevice& Device)
	: Device(Device)
{
}

void VulkanSurface::ReleaseGL()
{
	vkDestroySwapchainKHR(Device, Swapchain, nullptr);
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
			(uint32)GPlatform->GetWindowSize().x,
			(uint32)GPlatform->GetWindowSize().y
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

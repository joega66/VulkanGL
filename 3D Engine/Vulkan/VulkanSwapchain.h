#pragma once
#include "VulkanImage.h"

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;

	SwapchainSupportDetails(VkPhysicalDevice Device, VkSurfaceKHR Surface);
};

class VulkanSwapchain
{
public:
	/** SwapchainKHR. */
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

	/** Swapchain images. */
	std::vector<VulkanImageRef> Images;

	VulkanSwapchain() = default;

	/** Create a new swapchain (if within surface capabilities.) */
	void Create(class VulkanDevice& Device, uint32 ScreenWidth, uint32 ScreenHeight);

	/** Free the swapchain. */
	void Free();

	operator VkSwapchainKHR() { return Swapchain; }
};
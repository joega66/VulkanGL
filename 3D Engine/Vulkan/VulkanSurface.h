#pragma once
#include "VulkanImage.h"

class VulkanDevice;

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;

	void QuerySwapchainSupport(VkPhysicalDevice Device, VkSurfaceKHR Surface);
};

class VulkanSurface
{
public:
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
	std::vector<VulkanImageRef> Images;

	VulkanSurface(VulkanDevice& Device);

	void Init();
	void Free();

	operator VkSwapchainKHR() { return Swapchain; }

private:
	VulkanDevice& Device;

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvailablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& Capabilities);
};

CLASS(VulkanSurface);
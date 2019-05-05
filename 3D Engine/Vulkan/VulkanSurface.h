#pragma once
#include "VulkanRenderTargetView.h"

class VulkanDevice;

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;

	void QuerySwapchainSupport(VkPhysicalDevice Device, VkSurfaceKHR Surface);
};

class VulkanSurface : public GLRenderResource
{
public:
	VkSwapchainKHR Swapchain;
	std::vector<VulkanImageRef> Images;
	std::vector<VulkanRenderTargetViewRef> RTViews;

	VulkanSurface(VulkanDevice& Device);

	void Init();
	virtual void ReleaseGL() final;
	operator VkSwapchainKHR() { return Swapchain; }

private:
	VulkanDevice& Device;

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvailablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& Capabilities);
};

CLASS(VulkanSurface);
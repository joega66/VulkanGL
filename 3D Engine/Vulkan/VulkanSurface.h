#pragma once
#include "VulkanDRM.h"

class VulkanSurface : public drm::Surface
{
public:
	VulkanSurface(Platform& Platform, VulkanDevice& Device);

	void Init(VulkanDevice& Device);

	/** Get the next image index. */
	virtual uint32 AcquireNextImage(DRM& Device) override;

	/** Present the image to the display engine. */
	virtual void Present(DRM& Device, uint32 ImageIndex, drm::CommandListRef CmdList) override;

	/** Create a new swapchain (if within surface capabilities.) */
	virtual void Resize(DRM& Device, uint32 ScreenWidth, uint32 ScreenHeight) override;

	virtual drm::ImageRef GetImage(uint32 ImageIndex) override;

	virtual void GetSwapchainImages(uint32 ImageCount, drm::ImageRef* Images) override;

	operator VkSurfaceKHR() { return Surface; }

private:
	/** */
	VkSurfaceKHR Surface;

	/** SwapchainKHR. */
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

	/** Swapchain images. */
	std::vector<VulkanImageRef> Images;

	VkSemaphore ImageAvailableSem = VK_NULL_HANDLE;
	VkSemaphore RenderEndSem = VK_NULL_HANDLE;
};
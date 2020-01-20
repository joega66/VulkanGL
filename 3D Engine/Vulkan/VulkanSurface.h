#pragma once
#include "VulkanDRM.h"

class VulkanSurface : public drm::Surface
{
public:
	VulkanSurface(Platform& Platform, VulkanDRM& Device);

	/** Some initialization that needs to happen using the logical device. 
	  * This is kinda janky and will get removed in the future.
	  */
	void Init(VulkanDevice& Device);

	/** Get the next image index. */
	virtual uint32 AcquireNextImage(DRM& Device) override;

	/** Present the image to the display engine. */
	virtual void Present(DRM& Device, uint32 ImageIndex, drm::CommandListRef CmdList) override;

	/** Create a new swapchain (if within surface capabilities.) */
	virtual void Resize(DRM& Device, uint32 ScreenWidth, uint32 ScreenHeight) override;

	/** Get the swapchain image. */
	virtual drm::ImageRef GetImage(uint32 ImageIndex) override;

	operator VkSurfaceKHR() { return Surface; }

private:
	/** The surface. */
	VkSurfaceKHR Surface;

	/** The swapchain. */
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

	/** Swapchain images. */
	std::vector<VulkanImageRef> Images;

	/** @TODO Move me to the SceneRenderer. */
	VkSemaphore ImageAvailableSem = VK_NULL_HANDLE;
	VkSemaphore RenderEndSem = VK_NULL_HANDLE;

	/** Present queue. */
	VkQueue PresentQueue = VK_NULL_HANDLE;
	uint32 PresentIndex = -1;
};
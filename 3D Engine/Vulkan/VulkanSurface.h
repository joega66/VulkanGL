#pragma once
#include <GPU/GPU.h>
#include "VulkanImage.h"

class VulkanSurface : public gpu::Surface
{
public:
	VulkanSurface(Platform& Platform, VulkanDevice& Device);

	/** Some initialization that needs to happen using the logical device. 
	  * This is kinda janky and will get removed when gpu::Semaphore comes.
	  */
	void Init(VulkanDevice& Device);

	/** Get the next image index. */
	virtual uint32 AcquireNextImage(gpu::Device& Device) override;

	/** Present the image to the display engine. */
	virtual void Present(gpu::Device& Device, uint32 ImageIndex, gpu::CommandList& CmdList) override;

	/** Create a new swapchain (if within surface capabilities.) */
	virtual void Resize(gpu::Device& Device, uint32 ScreenWidth, uint32 ScreenHeight) override;

	/** Get the swapchain image. */
	virtual const gpu::Image& GetImage(uint32 ImageIndex) override;

	/** Get all images in the swapchain. */
	virtual const std::vector<gpu::Image>& GetImages() override;

	operator VkSurfaceKHR() { return Surface; }

	inline VkSurfaceFormatKHR GetFormat() const { return SurfaceFormat; }
	inline VkPresentModeKHR GetPresentMode() const { return PresentMode; }

private:
	VkSurfaceKHR Surface;

	VkSurfaceFormatKHR SurfaceFormat;

	VkPresentModeKHR PresentMode;

	/** The swapchain. */
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

	/** Swapchain images. */
	std::vector<gpu::Image> Images;

	/** @todo Move me to the SceneRenderer. */
	VkSemaphore ImageAvailableSem = VK_NULL_HANDLE;
	VkSemaphore RenderEndSem = VK_NULL_HANDLE;

	/** Present queue. */
	VkQueue PresentQueue = VK_NULL_HANDLE;
	uint32 PresentIndex = -1;
};
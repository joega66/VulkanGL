#pragma once
#include <GPU/GPU.h>
#include "VulkanImage.h"

class VulkanSurface : public gpu::Surface
{
public:
	VulkanSurface(VulkanDevice& device);

	/** Get the next image index. */
	uint32 AcquireNextImage() override;

	/** Present the image to the display engine. */
	void Present(uint32 imageIndex, gpu::CommandList& cmdList) override;

	/** Create a new swapchain (if within surface capabilities.) */
	void Resize(uint32 screenWidth, uint32 screenHeight, EImageUsage imageUsage) override;

	/** Get the swapchain image. */
	const gpu::Image& GetImage(uint32 imageIndex) override;

	/** Get all images in the swapchain. */
	const std::vector<gpu::Image>& GetImages() override;

	inline VkSurfaceFormatKHR GetFormat() const { return _SurfaceFormat; }
	inline VkPresentModeKHR GetPresentMode() const { return _PresentMode; }

private:
	VulkanDevice& _Device;

	VkSurfaceFormatKHR _SurfaceFormat;

	VkPresentModeKHR _PresentMode;

	/** The swapchain. */
	VkSwapchainKHR _Swapchain = VK_NULL_HANDLE;

	/** Swapchain images. */
	std::vector<gpu::Image> _Images;

	/** @todo Move me to the SceneRenderer. */
	VkSemaphore _ImageAvailableSem = VK_NULL_HANDLE;
	VkSemaphore _RenderEndSem = VK_NULL_HANDLE;
};
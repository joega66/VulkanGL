#pragma once
#include <GPU/GPU.h>
#include "VulkanImage.h"

class VulkanInstance;
class VulkanPhysicalDevice;

class VulkanCompositor : public gpu::Compositor
{
public:
	VulkanCompositor(VulkanInstance& instance, VulkanPhysicalDevice& physicalDevice, void* windowHandle);

	/** Get the next image index. */
	uint32 AcquireNextImage(gpu::Device& device) override;

	/** Present the image to the display engine. */
	void Present(gpu::Device& device, uint32 imageIndex, gpu::CommandList& cmdList) override;

	/** Create a new swapchain (if within surface capabilities.) */
	void Resize(gpu::Device& device, uint32 screenWidth, uint32 screenHeight, EImageUsage imageUsage) override;

	/** Get all images in the swapchain. */
	const std::vector<gpu::Image>& GetImages() override;
	
	inline VkSurfaceFormatKHR GetFormat() const { return _SurfaceFormat; }
	inline VkPresentModeKHR GetPresentMode() const { return _PresentMode; }
	inline uint32 GetPresentIndex() const { return _PresentIndex; }

private:
	VkSurfaceKHR _Surface;

	uint32 _PresentIndex = -1;

	VkQueue _PresentQueue = VK_NULL_HANDLE;

	VkSurfaceFormatKHR _SurfaceFormat;

	VkPresentModeKHR _PresentMode;

	VkSwapchainKHR _Swapchain;

	std::vector<gpu::Image> _Images;

	/** @todo Move me to the SceneRenderer. */
	VkSemaphore _ImageAvailableSem = VK_NULL_HANDLE;
	VkSemaphore _RenderEndSem = VK_NULL_HANDLE;
};
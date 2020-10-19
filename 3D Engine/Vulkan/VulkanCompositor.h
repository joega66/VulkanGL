#pragma once
#include <GPU/GPU.h>
#include "VulkanImage.h"

class VulkanInstance;
class VulkanPhysicalDevice;

class VulkanCompositor : public gpu::Compositor
{
public:
	VulkanCompositor(VulkanInstance& instance, VulkanPhysicalDevice& physicalDevice, void* windowHandle);

	uint32 AcquireNextImage(gpu::Device& device, gpu::Semaphore& semaphore) override;

	void QueuePresent(gpu::Device& device, uint32 imageIndex, gpu::Semaphore& waitSemaphore) override;

	void Resize(gpu::Device& device, uint32 screenWidth, uint32 screenHeight, EImageUsage imageUsage) override;

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
};
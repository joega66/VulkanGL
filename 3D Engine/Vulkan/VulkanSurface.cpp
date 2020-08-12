#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include <GLFW/glfw3.h>

static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR>& availableFormats,
	const VkSurfaceFormatKHR& bestSurfaceFormat)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == bestSurfaceFormat.format && availableFormat.colorSpace == bestSurfaceFormat.colorSpace)
		{
			return availableFormat;
		}
	}

	return availableFormats.front();
}

static VkPresentModeKHR ChooseSwapPresentMode(
	const std::vector<VkPresentModeKHR>& availablePresentModes,
	VkPresentModeKHR bestMode)
{
	const VkPresentModeKHR rankedPresentModes[] =
	{
		bestMode,
		VK_PRESENT_MODE_MAILBOX_KHR,
		VK_PRESENT_MODE_IMMEDIATE_KHR
	};

	for (const auto presentMode : rankedPresentModes)
	{
		if (std::find(availablePresentModes.begin(), availablePresentModes.end(), presentMode) != availablePresentModes.end())
		{
			return presentMode;
		}
	}

	return availablePresentModes.front();
}

static VkExtent2D ChooseSwapExtent(uint32 width, uint32 height, const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		return VkExtent2D
		{
			std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, width)),
			std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, height))
		};
	}
}

VulkanSurface::VulkanSurface(Platform& platform, VulkanDevice& device)
	: _Device(device)
{
	vulkan(glfwCreateWindowSurface(_Device.GetInstance(), platform.Window, nullptr, &_Surface));

	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_Device.GetPhysicalDevice(), &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(_Device.GetPhysicalDevice(), &queueFamilyCount, queueFamilies.data());

	// Find the present index.
	for (uint32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilies.size(); queueFamilyIndex++)
	{
		const VkQueueFamilyProperties& queueFamily = queueFamilies[queueFamilyIndex];

		VkBool32 hasPresentSupport = false;
		vulkan( vkGetPhysicalDeviceSurfaceSupportKHR(_Device.GetPhysicalDevice(), queueFamilyIndex, _Surface, &hasPresentSupport) );

		if (hasPresentSupport)
		{
			_PresentIndex = queueFamilyIndex;
			break;
		}
	}

	check(_PresentIndex != -1, "No present family index found!");

	_Device.GetQueues().RequestQueueFamily(_PresentIndex);
}

uint32 VulkanSurface::AcquireNextImage()
{
	uint32 imageIndex;

	if (const VkResult result = vkAcquireNextImageKHR(_Device,
		_Swapchain,
		std::numeric_limits<uint32>::max(),
		_ImageAvailableSem,
		VK_NULL_HANDLE,
		&imageIndex); result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		signal_unimplemented();
	}
	else
	{
		vulkan(result);
		check(imageIndex >= 0 && imageIndex < _Images.size(), "Error acquiring swapchain.");
	}

	return imageIndex;
}

void VulkanSurface::Present(uint32 imageIndex, gpu::CommandList& cmdList)
{
	vulkan(vkEndCommandBuffer(cmdList._CommandBuffer));

	const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &_ImageAvailableSem;
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdList._CommandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &_RenderEndSem;

	vulkan(vkQueueSubmit(cmdList._Queue, 1, &submitInfo, VK_NULL_HANDLE));

	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.pWaitSemaphores = &_RenderEndSem;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pSwapchains = &_Swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pImageIndices = &imageIndex;

	if (const VkResult result = vkQueuePresentKHR(_PresentQueue, &presentInfo); result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		signal_unimplemented();
	}
	else
	{
		vulkan(result);
	}

	vulkan(vkQueueWaitIdle(_PresentQueue));
}

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	SwapchainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		vulkan( vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities) );

		uint32 formatCount;
		vulkan( vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr) );

		check( formatCount != 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned 0 formats." );

		formats.resize(formatCount);
		vulkan( vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data()) );
	
		uint32 presentModeCount;
		vulkan( vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr) );

		check( presentModeCount != 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned 0 present modes." );

		presentModes.resize(presentModeCount);
		vulkan( vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data()) );
	}
};

void VulkanSurface::Resize(uint32 screenWidth, uint32 screenHeight, EImageUsage imageUsage)
{
	if (_PresentQueue == VK_NULL_HANDLE)
	{
		vkGetDeviceQueue(_Device, _PresentIndex, 0, &_PresentQueue);
	}

	if (_ImageAvailableSem == VK_NULL_HANDLE)
	{
		VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		vulkan(vkCreateSemaphore(_Device, &semaphoreInfo, nullptr, &_ImageAvailableSem));
		vulkan(vkCreateSemaphore(_Device, &semaphoreInfo, nullptr, &_RenderEndSem));
	}
	
	const SwapchainSupportDetails swapchainSupport(_Device.GetPhysicalDevice(), _Surface);

	_SurfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats, { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR });

	_PresentMode = ChooseSwapPresentMode(swapchainSupport.presentModes, VK_PRESENT_MODE_FIFO_KHR);

	const VkExtent2D extent = ChooseSwapExtent(screenWidth, screenHeight, swapchainSupport.capabilities);

	uint32 imageCount = swapchainSupport.capabilities.minImageCount + 1;

	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
	{
		imageCount = swapchainSupport.capabilities.maxImageCount;
	}
	
	VkSwapchainCreateInfoKHR swapchainInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainInfo.surface = _Surface;
	swapchainInfo.minImageCount = imageCount;
	swapchainInfo.imageFormat = _SurfaceFormat.format;
	swapchainInfo.imageColorSpace = _SurfaceFormat.colorSpace;
	swapchainInfo.imageExtent = extent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = [&] ()
	{
		VkImageUsageFlags imageUsageFlags;
		imageUsageFlags |= Any(imageUsage & EImageUsage::Attachment) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
		imageUsageFlags |= Any(imageUsage & EImageUsage::Sampled) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
		imageUsageFlags |= Any(imageUsage & EImageUsage::Storage) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
		imageUsageFlags |= Any(imageUsage & EImageUsage::TransferSrc) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
		imageUsageFlags |= Any(imageUsage & EImageUsage::TransferDst) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		return imageUsageFlags;
	}();

	const uint32 queueFamilyIndices[] = 
	{ 
		static_cast<uint32>(_Device.GetQueues().GetGraphicsIndex()),
		static_cast<uint32>(_PresentIndex)
	};

	if (queueFamilyIndices[0] != queueFamilyIndices[1])
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = 2;
		swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkSwapchainKHR oldSwapchain = _Swapchain;

	swapchainInfo.preTransform = swapchainSupport.capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = _PresentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = oldSwapchain;

	vulkan(vkCreateSwapchainKHR(_Device, &swapchainInfo, nullptr, &_Swapchain));

	vkGetSwapchainImagesKHR(_Device, _Swapchain, &imageCount, nullptr);

	std::vector<VkImage> images(imageCount);

	vkGetSwapchainImagesKHR(_Device, _Swapchain, &imageCount, images.data());
	
	_Images.clear();
	_Images.reserve(imageCount);

	for (auto& image : images)
	{
		_Images.push_back(gpu::Image(_Device
			, image
			, VkDeviceMemory()
			, VulkanImage::GetEngineFormat(_SurfaceFormat.format)
			, extent.width
			, extent.height
			, 1
			, imageUsage
			, 1
		));
	}
}

const gpu::Image& VulkanSurface::GetImage(uint32 imageIndex)
{
	return _Images[imageIndex];
}

const std::vector<gpu::Image>& VulkanSurface::GetImages()
{
	return _Images;
}

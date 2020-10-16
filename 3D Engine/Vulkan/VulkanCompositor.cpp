#include "VulkanCompositor.h"
#include "VulkanInstance.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"

#if _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#undef max
#undef min
#endif

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

VulkanCompositor::VulkanCompositor(VulkanInstance& instance, VulkanPhysicalDevice& physicalDevice, void* windowHandle)
{
#if _WIN32
	auto vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));

	check(vkCreateWin32SurfaceKHR, "Failed to get proc address vkCreateWin32SurfaceKHR");

	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
	surfaceCreateInfo.hwnd = static_cast<HWND>(windowHandle);

	vulkan(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &_Surface));
#endif

	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (uint32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilies.size(); queueFamilyIndex++)
	{
		VkBool32 hasPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, _Surface, &hasPresentSupport);

		if (hasPresentSupport)
		{
			_PresentIndex = queueFamilyIndex;
			break;
		}
	}

	check(_PresentIndex != -1, "No present index found!!");
}

uint32 VulkanCompositor::AcquireNextImage(gpu::Device& device)
{
	uint32 imageIndex;

	if (const VkResult result = vkAcquireNextImageKHR(static_cast<VulkanDevice&>(device),
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

void VulkanCompositor::Present(gpu::Device& device, uint32 imageIndex, gpu::CommandList& cmdList)
{
	auto& _Device = static_cast<VulkanDevice&>(device);

	vulkan(vkEndCommandBuffer(cmdList._CommandBuffer));

	// @todo This is super duper nasty
	_Device.GetQueues().GetQueue(EQueue::Transfer).WaitIdle(_Device);

	const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	const VkSubmitInfo submitInfo = 
	{ 
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &_ImageAvailableSem,
		.pWaitDstStageMask = &waitDstStageMask,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdList._CommandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &_RenderEndSem,
	};
	
	vulkan(vkQueueSubmit(cmdList._Queue.GetQueue(), 1, &submitInfo, VK_NULL_HANDLE));

	const VkPresentInfoKHR presentInfo = 
	{ 
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &_RenderEndSem,
		.swapchainCount = 1,
		.pSwapchains = &_Swapchain,
		.pImageIndices = &imageIndex,
	};

	if (_PresentQueue == VK_NULL_HANDLE)
	{
		vkGetDeviceQueue(_Device, _PresentIndex, 0, &_PresentQueue);
	}

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

void VulkanCompositor::Resize(gpu::Device& device, uint32 screenWidth, uint32 screenHeight, EImageUsage imageUsage)
{
	auto& _Device = static_cast<VulkanDevice&>(device);

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
		VkImageUsageFlags imageUsageFlags = 0;
		imageUsageFlags |= Any(imageUsage & EImageUsage::Attachment) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
		imageUsageFlags |= Any(imageUsage & EImageUsage::Sampled) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
		imageUsageFlags |= Any(imageUsage & EImageUsage::Storage) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
		imageUsageFlags |= Any(imageUsage & EImageUsage::TransferSrc) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
		imageUsageFlags |= Any(imageUsage & EImageUsage::TransferDst) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		return imageUsageFlags;
	}();

	const uint32 queueFamilyIndices[] = 
	{ 
		static_cast<uint32>(_Device.GetQueues().GetQueue(EQueue::Graphics).GetQueueFamilyIndex()),
		_PresentIndex
	};

	if (queueFamilyIndices[0] != queueFamilyIndices[1])
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = static_cast<uint32>(std::size(queueFamilyIndices));
		swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	const VkSwapchainKHR oldSwapchain = _Swapchain;

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
		_Images.push_back(gpu::Image(
			_Device,
			nullptr,
			nullptr,
			{},
			image,
			gpu::Image::GetEngineFormat(_SurfaceFormat.format),
			extent.width,
			extent.height,
			1,
			imageUsage,
			1
		));
	}
}

const std::vector<gpu::Image>& VulkanCompositor::GetImages()
{
	return _Images;
}

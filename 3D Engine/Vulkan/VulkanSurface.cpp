#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include <GLFW/glfw3.h>

static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats)
{
	if (AvailableFormats.size() == 1 && AvailableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& AvailableFormat : AvailableFormats)
	{
		if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return AvailableFormat;
		}
	}

	return AvailableFormats[0];
}

static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvailablePresentModes)
{
	VkPresentModeKHR BestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& AvailablePresentMode : AvailablePresentModes)
	{
		if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return AvailablePresentMode;
		}
		else if (AvailablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			BestMode = AvailablePresentMode;
		}
	}

	return BestMode;
}

static VkExtent2D ChooseSwapExtent(uint32 Width, uint32 Height, const VkSurfaceCapabilitiesKHR& Capabilities)
{
	if (Capabilities.currentExtent.width != std::numeric_limits<uint32>::max())
	{
		return Capabilities.currentExtent;
	}
	else
	{
		VkExtent2D ActualExtent = { Width, Height };
		ActualExtent.width = std::max(Capabilities.minImageExtent.width, std::min(Capabilities.maxImageExtent.width, ActualExtent.width));
		ActualExtent.height = std::max(Capabilities.minImageExtent.height, std::min(Capabilities.maxImageExtent.height, ActualExtent.height));
		return ActualExtent;
	}
}

VulkanSurface::VulkanSurface(Platform& Platform, VulkanDevice& Device)
{
	vulkan(glfwCreateWindowSurface(Device.GetInstance(), Platform.Window, nullptr, &Surface));

	uint32 QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device.GetPhysicalDevice(), &QueueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Device.GetPhysicalDevice(), &QueueFamilyCount, QueueFamilies.data());

	// Find the present index.
	for (int32 QueueFamilyIndex = 0; QueueFamilyIndex < static_cast<int32>(QueueFamilies.size()); QueueFamilyIndex++)
	{
		const VkQueueFamilyProperties& QueueFamily = QueueFamilies[QueueFamilyIndex];

		VkBool32 HasPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(Device.GetPhysicalDevice(), QueueFamilyIndex, Surface, &HasPresentSupport);

		if (HasPresentSupport)
		{
			PresentIndex = QueueFamilyIndex;
			break;
		}
	}

	check(PresentIndex != -1, "No present family index found!");

	Device.GetQueues().RequestQueueFamily(PresentIndex);
}

void VulkanSurface::Init(VulkanDevice& Device)
{
	VkSemaphoreCreateInfo SemaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vulkan(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &ImageAvailableSem));
	vulkan(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &RenderEndSem));

	vkGetDeviceQueue(Device, PresentIndex, 0, &PresentQueue);
}

uint32 VulkanSurface::AcquireNextImage(DRMDevice& Device)
{
	VulkanDevice& VulkanDevice = static_cast<class VulkanDevice&>(Device);

	uint32 ImageIndex;

	if (VkResult Result = vkAcquireNextImageKHR(VulkanDevice,
		Swapchain,
		std::numeric_limits<uint32>::max(),
		ImageAvailableSem,
		VK_NULL_HANDLE,
		&ImageIndex); Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		signal_unimplemented();
	}
	else
	{
		vulkan(Result);
		check(ImageIndex >= 0 && ImageIndex < Images.size(), "Error acquiring swapchain.");
	}

	return ImageIndex;
}

void VulkanSurface::Present(DRMDevice& Device, uint32 ImageIndex, drm::CommandList& CmdList)
{
	VulkanDevice& VulkanDevice = static_cast<class VulkanDevice&>(Device);

	vulkan(vkEndCommandBuffer(CmdList.CommandBuffer));

	const VkPipelineStageFlags WaitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores = &ImageAvailableSem;
	SubmitInfo.pWaitDstStageMask = &WaitDstStageMask;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CmdList.CommandBuffer;
	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.pSignalSemaphores = &RenderEndSem;

	vulkan(vkQueueSubmit(CmdList.Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

	VkPresentInfoKHR PresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	PresentInfo.pWaitSemaphores = &RenderEndSem;
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pSwapchains = &Swapchain;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pImageIndices = &ImageIndex;

	if (VkResult Result = vkQueuePresentKHR(PresentQueue, &PresentInfo); Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
	{
		signal_unimplemented();
	}
	else
	{
		vulkan(Result);
	}

	vulkan(vkQueueWaitIdle(PresentQueue));
}

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;

	SwapchainSupportDetails(VkPhysicalDevice Device, VkSurfaceKHR Surface)
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, Surface, &Capabilities);

		uint32 FormatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, nullptr);

		if (FormatCount != 0)
		{
			Formats.resize(FormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, Formats.data());
		}

		uint32 PresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, nullptr);

		if (PresentModeCount != 0)
		{
			PresentModes.resize(PresentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, PresentModes.data());
		}
	}
};

void VulkanSurface::Resize(DRMDevice& Device, uint32 Width, uint32 Height)
{
	VulkanDevice& VulkanDevice = static_cast<class VulkanDevice&>(Device);

	const SwapchainSupportDetails SwapchainSupport(VulkanDevice.GetPhysicalDevice(), Surface);

	SurfaceFormat = ChooseSwapSurfaceFormat(SwapchainSupport.Formats);

	PresentMode = ChooseSwapPresentMode(SwapchainSupport.PresentModes);

	const VkExtent2D Extent = ChooseSwapExtent(Width, Height, SwapchainSupport.Capabilities);
	uint32 ImageCount = SwapchainSupport.Capabilities.minImageCount + 1;

	if (SwapchainSupport.Capabilities.maxImageCount > 0 && ImageCount > SwapchainSupport.Capabilities.maxImageCount)
	{
		ImageCount = SwapchainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR SwapchainInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	SwapchainInfo.surface = Surface;
	SwapchainInfo.minImageCount = ImageCount;
	SwapchainInfo.imageFormat = SurfaceFormat.format;
	SwapchainInfo.imageColorSpace = SurfaceFormat.colorSpace;
	SwapchainInfo.imageExtent = Extent;
	SwapchainInfo.imageArrayLayers = 1;
	SwapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	const uint32 QueueFamilyIndices[] = 
	{ 
		static_cast<uint32>(VulkanDevice.GetQueues().GetGraphicsIndex()),
		static_cast<uint32>(PresentIndex)
	};

	if (QueueFamilyIndices[0] != QueueFamilyIndices[1])
	{
		SwapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		SwapchainInfo.queueFamilyIndexCount = 2;
		SwapchainInfo.pQueueFamilyIndices = QueueFamilyIndices;
	}
	else
	{
		SwapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkSwapchainKHR OldSwapchain = Swapchain;

	SwapchainInfo.preTransform = SwapchainSupport.Capabilities.currentTransform;
	SwapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapchainInfo.presentMode = PresentMode;
	SwapchainInfo.clipped = VK_TRUE;
	SwapchainInfo.oldSwapchain = OldSwapchain;

	vulkan(vkCreateSwapchainKHR(VulkanDevice, &SwapchainInfo, nullptr, &Swapchain));

	vkGetSwapchainImagesKHR(VulkanDevice, Swapchain, &ImageCount, nullptr);

	std::vector<VkImage> VulkanImages(ImageCount);

	vkGetSwapchainImagesKHR(VulkanDevice, Swapchain, &ImageCount, VulkanImages.data());
	
	Images.clear();
	Images.reserve(ImageCount);

	for (auto& VulkanImage : VulkanImages)
	{
		Images.push_back(
			MakeRef<class VulkanImage>(VulkanDevice
				, VulkanImage
				, VkDeviceMemory()
				, VulkanImage::GetEngineFormat(SurfaceFormat.format)
				, Extent.width
				, Extent.height
				, 1
				, EImageUsage::Attachment | EImageUsage::TransferDst)
		);
	}
}

drm::ImageRef VulkanSurface::GetImage(uint32 ImageIndex)
{
	return Images[ImageIndex];
}
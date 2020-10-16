#include "VulkanPhysicalDevice.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"

static bool AllDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const std::vector<const char*>& deviceExtensions)
{
	uint32 extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance& instance)
{
	uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	check(deviceCount != 0, "Failed to find GPUs with Vulkan support.");

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

	const auto& deviceExtensions = VulkanDevice::GetRequiredExtensions();

	for (const auto& physicalDevice : physicalDevices)
	{
		if (AllDeviceExtensionsSupported(physicalDevice, deviceExtensions))
		{
			_PhysicalDevice = physicalDevice;
			break;
		}
	}

	check(_PhysicalDevice != VK_NULL_HANDLE, "Failed to find a GPU.");

	vkGetPhysicalDeviceProperties(_PhysicalDevice, &_Properties);

	vkGetPhysicalDeviceFeatures(_PhysicalDevice, &_Features);
}

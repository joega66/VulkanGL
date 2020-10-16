#pragma once
#include <vulkan/vulkan.h>

class VulkanInstance;

class VulkanPhysicalDevice
{
public:
	VulkanPhysicalDevice(VulkanInstance& instance);

	inline operator const VkPhysicalDevice&() const { return _PhysicalDevice; }
	inline const VkPhysicalDeviceProperties& GetProperties() const { return _Properties; }
	inline const VkPhysicalDeviceFeatures& GetFeatures() const { return _Features; }

private:
	VkPhysicalDevice _PhysicalDevice;

	VkPhysicalDeviceProperties _Properties;

	VkPhysicalDeviceFeatures _Features;
};
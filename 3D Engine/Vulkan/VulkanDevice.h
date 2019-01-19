#pragma once
#include <Platform/Platform.h>
#include <vulkan/vulkan.h>
#include <sstream>
#include <iostream>

class VulkanDevice
{
public:
	VkSurfaceKHR Surface;
	VkQueue GraphicsQueue;
	VkQueue PresentQueue;
	VkCommandPool CommandPool;
	VkPhysicalDeviceProperties Properties;
	VkPhysicalDeviceFeatures Features;

	VulkanDevice();
	operator VkDevice() { return Device; }
	operator VkPhysicalDevice() { return PhysicalDevice; }

private:
	VkInstance Instance;
	VkDebugReportCallbackEXT DebugReportCallback;
	VkDevice Device;
	VkPhysicalDevice PhysicalDevice;
};

CLASS(VulkanDevice);

struct QueueFamilyIndices
{
	int GraphicsFamily = -1;
	int PresentFamily = -1;

	bool IsComplete() const;
	void FindQueueFamilies(VkPhysicalDevice Device, VkSurfaceKHR Surface);
};

void VulkanAssert(VkResult Result, const char* Func, const char* File, int Line);

#define vulkan(Result) \
		if (Result != VK_SUCCESS)	\
		{							\
			std::stringstream SS;	\
			SS << "Vulkan call ";	\
			SS << #Result;			\
			SS << " in file ";		\
			SS << __FILE__;			\
			SS << " on line ";		\
			SS << __LINE__;			\
			SS << " failed.";		\
			fail(SS.str());			\
		}							
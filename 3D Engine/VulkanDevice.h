#pragma once
#include "GHIDevice.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDevice : public GHIDevice
{
public:
	virtual void Open() override;
	virtual void Close() override;

	operator VkDevice() { return Device; }
	operator VkPhysicalDevice() { return PhysicalDevice; }

private:
	GLFWwindow* Window;
	VkDevice Device;
	VkPhysicalDevice PhysicalDevice;

	void OpenWindow(int Width, int Height);
};
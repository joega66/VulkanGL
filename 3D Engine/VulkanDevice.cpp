#include "VulkanDevice.h"

void VulkanDevice::Open()
{
	OpenWindow(1280, 960);


}

void VulkanDevice::Close()
{

}

void VulkanDevice::OpenWindow(int Width, int Height)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Window = glfwCreateWindow(Width, Height, "3D Engine", nullptr, nullptr);

	glfwSetKeyCallback(Window, App::KeyboardCallback);
	glfwSetCursorPosCallback(Window, App::MouseCallback);
	glfwSetScrollCallback(Window, App::ScrollCallback);
	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(Window, this);
	glfwSetWindowSizeCallback(Window, VulkanRenderer::onWindowResized);
}

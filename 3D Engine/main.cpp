#include <Engine/Engine.h>
#include <Vulkan/VulkanDevice.h>
#include <Vulkan/VulkanShaderLibrary.h>
#include <Vulkan/VulkanSurface.h>
#include <Engine/Screen.h>
#include <Engine/Input.h>
#include <Engine/Cursor.h>
#include <GLFW/glfw3.h>

int main(int argc, char* argv[])
{
	Platform Platform(
		Platform::GetInt("Engine.ini", "Renderer", "WindowSizeX", 720), 
		Platform::GetInt("Engine.ini", "Renderer", "WindowSizeY", 720)
	);

	Cursor Cursor(Platform);
	Input Input(Platform);
	Screen Screen(Platform);

	GLFWWindowUserPointer WindowUserPointer{ &Cursor, &Input, &Screen };
	glfwSetWindowUserPointer(Platform.Window, &WindowUserPointer);
	
	DeviceDesc deviceDesc = {};
	deviceDesc.windowHandle = Platform.GetWindow();
	deviceDesc.enableValidationLayers = Platform::GetBool("Engine.ini", "Renderer", "UseValidationLayers", false);

	std::unique_ptr<VulkanDevice> Device = std::make_unique<VulkanDevice>(deviceDesc);
	std::unique_ptr<VulkanSurface> Surface = std::make_unique<VulkanSurface>(*Device);
	std::unique_ptr<VulkanShaderLibrary> ShaderLibrary = std::make_unique<VulkanShaderLibrary>(*Device);

	Engine Engine(Platform, Cursor, Input, Screen, *Device, *ShaderLibrary, *Surface);
	Engine.Main();

	return 0;
}
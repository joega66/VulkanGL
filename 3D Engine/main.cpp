#include <Engine/Engine.h>
#include <Vulkan/VulkanInstance.h>
#include <Vulkan/VulkanPhysicalDevice.h>
#include <Vulkan/VulkanDevice.h>
#include <Vulkan/VulkanShaderLibrary.h>
#include <Vulkan/VulkanCompositor.h>
#include <Engine/Screen.h>
#include <Engine/Input.h>
#include <Engine/Cursor.h>
#include <GLFW/glfw3.h>

int main(int argc, char* argv[])
{
	Platform platform(
		Platform::GetInt("Engine.ini", "Renderer", "WindowSizeX", 720),
		Platform::GetInt("Engine.ini", "Renderer", "WindowSizeY", 720)
	);

	Cursor cursor(platform);
	Input input(platform);
	Screen screen(platform);

	GLFWWindowUserPointer windowUserPointer{ &cursor, &input, &screen };
	glfwSetWindowUserPointer(platform._Window, &windowUserPointer);

	VulkanInstance instance(Platform::GetBool("Engine.ini", "Renderer", "UseValidationLayers", false));
	VulkanPhysicalDevice physicalDevice(instance);
	VulkanCompositor compositor(instance, physicalDevice, platform.GetWindow());
	VulkanDevice device(instance, physicalDevice, { compositor.GetPresentIndex() });
	VulkanShaderLibrary shaderLibrary(device);

	Engine engine(platform, cursor, input, screen, device, shaderLibrary, compositor);
	engine.Main();

	return 0;
}
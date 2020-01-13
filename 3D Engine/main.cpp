#include "EngineMain.h"
#include <Platform/Platform.h>
#include <Vulkan/VulkanDRM.h>
#include <vulkan/VulkanShaderMap.h>
#include <Engine/Screen.h>
#include <Engine/Input.h>
#include <Engine/Cursor.h>
#include <GLFW/glfw3.h>

int main(int argc, char* argv[])
{
	const int32 WindowSizeX = Platform::GetInt("Engine.ini", "Renderer", "WindowSizeX", 720);
	const int32 WindowSizeY = Platform::GetInt("Engine.ini", "Renderer", "WindowSizeY", 720);

	Platform Platform(WindowSizeX, WindowSizeY);
	Cursor Cursor(Platform);
	Input Input(Platform);
	Screen Screen(Platform);

	GLFWWindowUserPointer WindowUserPointer{ &Cursor, &Input, &Screen };
	glfwSetWindowUserPointer(Platform.Window, &WindowUserPointer);

	std::unique_ptr<VulkanDRM> Device = std::make_unique<VulkanDRM>(Platform, Screen);
	std::unique_ptr<VulkanShaderMap> ShaderMap = std::make_unique<VulkanShaderMap>(Device->Device);

	EngineMain EngineMain;
	EngineMain.Main(Platform, Cursor, Input, Screen, *Device, *ShaderMap);
	
	return 0;
}
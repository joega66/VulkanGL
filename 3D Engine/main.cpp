#include <Platform/Platform.h>
#include <Vulkan/VulkanDRM.h>
#include <vulkan/VulkanShaderMap.h>
#include "EngineMain.h"
#include <Engine/Screen.h>

int main(int argc, char* argv[])
{
	const int32 WindowResolutionX = Platform.GetInt("Engine.ini", "Renderer", "WindowResolutionX", 720);
	const int32 WindowResolutionY = Platform.GetInt("Engine.ini", "Renderer", "WindowResolutionY", 720);

	Platform.OpenWindow(WindowResolutionX, WindowResolutionY);

	Screen Screen;

	std::unique_ptr<VulkanDRM> Device = std::make_unique<VulkanDRM>(Screen);
	std::unique_ptr<VulkanShaderMap> ShaderMap = std::make_unique<VulkanShaderMap>(Device->Device);

	EngineMain EngineMain;
	EngineMain.Main(Screen, *Device, *ShaderMap);
	
	return 0;
}
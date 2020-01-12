#include <Platform/Platform.h>
#include <Vulkan/VulkanDRM.h>
#include <vulkan/VulkanShaderMap.h>
#include "CoreEngine.h"

int main(int argc, char* argv[])
{
	const int32 WindowResolutionX = Platform.GetInt("Engine.ini", "Renderer", "WindowResolutionX", 720);
	const int32 WindowResolutionY = Platform.GetInt("Engine.ini", "Renderer", "WindowResolutionY", 720);

	Platform.OpenWindow(WindowResolutionX, WindowResolutionY);

	std::unique_ptr<VulkanDRM> Device = std::make_unique<VulkanDRM>();
	std::unique_ptr<VulkanShaderMap> ShaderMap = std::make_unique<VulkanShaderMap>(Device->Device);

	CoreEngine CoreEngine;
	CoreEngine.Run(*Device, *ShaderMap);
	
	return 0;
}
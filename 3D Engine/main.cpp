#include <DRMPrivate.h>
#include <Platform/Platform.h>
#include <Vulkan/VulkanDRM.h>
#include <vulkan/VulkanShaderMap.h>
#include "CoreEngine.h"

int main(int argc, char* argv[])
{
	const int32 WindowResolutionX = Platform.GetInt("Engine.ini", "Renderer", "WindowResolutionX", 720);
	const int32 WindowResolutionY = Platform.GetInt("Engine.ini", "Renderer", "WindowResolutionY", 720);

	Platform.OpenWindow(WindowResolutionX, WindowResolutionY);

	GDRM = MakeRef<VulkanDRM>();
	std::unique_ptr<VulkanShaderMap> ShaderMap = std::make_unique<VulkanShaderMap>(std::static_pointer_cast<VulkanDRM>(GDRM)->Device);

	CoreEngine CoreEngine;
	CoreEngine.Run(*ShaderMap);
	
	return 0;
}
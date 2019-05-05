#include <Platform/WindowsPlatform.h>
#include <Vulkan/VulkanDRM.h>
#include "CoreEngine.h"
#include <cxxopts.hpp>

int main(int argc, char* argv[])
{
	cxxopts::Options Options("VulkanGL", "A Vulkan-based framework for graphics demos :)");

	Options.add_options()
		("vulkan", "Enable Vulkan graphics library")
		("w,width", "Window width", cxxopts::value<int32>())
		("h,height", "Window height", cxxopts::value<int32>());

	cxxopts::ParseResult Result = Options.parse(argc, argv);

	int32 WinX = Result["width"].as<int32>();
	int32 WinY = Result["height"].as<int32>();

#ifdef _WIN32
	GPlatform = MakeRef<WindowsPlatform>();
#endif

	GPlatform->OpenWindow(WinX, WinY);

	if (Result.count("vulkan"))
	{
		GDRM = MakeRef<VulkanDRM>();
	}

	// @todo-joe 
	// 2. Vulkan Semaphores
	// 4. Store shader metadata in Vulkan compiler / allow DRM shaders to be subclassed!!

	GDRM->Init();

	CoreEngine CoreEngine;
	CoreEngine.Run();

	GDRM->Release();

	return 0;
}
#include <DRMPrivate.h>
#include <Platform/Platform.h>
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

	Platform.OpenWindow(WinX, WinY);

	if (Result.count("vulkan"))
	{
		GDRM = MakeRef<VulkanDRM>();
	}

	GDRM->Init();

	CoreEngine CoreEngine;
	CoreEngine.Run();

	GDRM->Release();

	return 0;
}
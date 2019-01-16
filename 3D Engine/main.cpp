#include <Platform/WindowsPlatform.h>
#include <Vulkan/VulkanGL.h>
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

	// @todo Platform does not have to be virtual...
	// Will also make the Vulkan surface creation far less hacky.
#ifdef _WIN32
	GPlatform = MakeRef<WindowsPlatform>();
#endif

	GPlatform->OpenWindow(WinX, WinY);

	if (Result.count("vulkan"))
	{
		GRender = MakeRef<VulkanGL>();
		GShaderCompiler = MakeRef<VulkanShaderCompiler>();
	}

	GRender->InitGL();

	CoreEngine CoreEngine;
	CoreEngine.Run();

	GRender->ReleaseGL();

	return 0;
}
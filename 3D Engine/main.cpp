#include "WindowsPlatform.h"
#include "VulkanGL.h"
#include "VulkanShader.h"
#include "FullscreenQuad.h"
#include "SceneRenderTargets.h"
#include <cxxopts.hpp>

void RunEngine()
{
	auto& SceneRenderTargets = SceneRenderTargets::Get();

	GPlatform->AddWindowListener(&SceneRenderTargets);

	COMPILE_SHADER(FullscreenVS, "C:/Users/Joe/Source/Repos/Shaders/FullscreenVS.glsl", "main", EShaderStage::Vertex);
	COMPILE_SHADER(FullscreenFS, "C:/Users/Joe/Source/Repos/Shaders/FullscreenFS.glsl", "main", EShaderStage::Fragment);
	COMPILE_SHADER(SunFS, "C:/Users/Joe/Source/Repos/Shaders/SunFS.glsl", "main", EShaderStage::Fragment);

	GLShaderRef VertexShader = GLCreateShader<FullscreenVS>();
	GLShaderRef FragmentShader = GLCreateShader<FullscreenFS>();
	GLShaderRef SunShader = GLCreateShader<SunFS>();

	std::array<float, 4> ClearColor = { 0, 0, 0, 0 };

	GLImageRef Depth = GLCreateImage(GPlatform->GetWindowSize().x, GPlatform->GetWindowSize().y, IF_D32_SFLOAT, RF_RenderTargetable);
	GLRenderTargetViewRef DepthView = GLCreateRenderTargetView(Depth, ELoadAction::Clear, EStoreAction::Store, 1.0f, 0);

	GLImageRef SunImage = GLCreateImage(GPlatform->GetWindowSize().x, GPlatform->GetWindowSize().y,
		IF_R8G8B8A8_SRGB, RF_RenderTargetable | RF_ShaderResource);
	GLRenderTargetViewRef SunView = GLCreateRenderTargetView(SunImage, ELoadAction::Clear, EStoreAction::Store, ClearColor);

	while (!GPlatform->WindowShouldClose())
	{
		GPlatform->PollEvents();

		GLBeginRender();

		/** Render to image */
		GLSetRenderTargets(1, &SunView, nullptr, DS_None);
		GLSetViewport(0.0f, 0.0f, (float)GPlatform->GetWindowSize().x, (float)GPlatform->GetWindowSize().y);
		GLSetDepthTest(false);
		GLSetColorMask(0, Color_RGBA);
		GLSetRasterizerState(CM_None);
		GLSetGraphicsPipeline(
			VertexShader,
			nullptr,
			nullptr,
			nullptr,
			SunShader
		);
		GLDraw(3, 1, 0, 0);

		/** Sample the image */
		GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Clear, EStoreAction::Store, ClearColor);
		GLSetRenderTargets(1, &SurfaceView, nullptr, DS_None);
		GLSetGraphicsPipeline(
			VertexShader,
			nullptr,
			nullptr,
			nullptr,
			FragmentShader
		);
		GLSetShaderResource(FragmentShader, 0, SunImage, SamplerState());
		GLDraw(3, 1, 0, 0);

		GLEndRender();
	}
}

int main(int argc, char* argv[])
{
	cxxopts::Options Options("VulkanGL", "A work-in-progress Vulkan wrapper :)");

	Options.add_options()
		("vulkan", "Enable Vulkan graphics library interface")
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
		GRender = MakeRef<VulkanGL>();
		GShaderCompiler = MakeRef<VulkanShaderCompiler>();
	}
	
	GRender->InitGL();
	RunEngine();
	GRender->ReleaseGL();

	return 0;
}
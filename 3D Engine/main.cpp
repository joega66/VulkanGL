#include "WindowsPlatform.h"
#include "VulkanGL.h"
#include "GLShader.h"
#include "FullscreenQuad.h"
#include "SceneRenderTargets.h"
#include <cxxopts.hpp>

void RunEngine()
{
	{
		ScopedAsyncShaderCompiler Compiler;
		Compiler.Compile<TestVS>("../Shaders/TestVS.glsl", "main", EShaderStage::Vertex);
		Compiler.Compile<FullscreenVS>("../Shaders/FullscreenVS.glsl", "main", EShaderStage::Vertex);
		Compiler.Compile<FullscreenFS>("../Shaders/FullscreenFS.glsl", "main", EShaderStage::Fragment);
		Compiler.Compile<RenderTargetFS>("../Shaders/SunFS.glsl", "main", EShaderStage::Fragment);
	}

	auto& SceneRenderTargets = SceneRenderTargets::Get();

	GPlatform->AddWindowListener(&SceneRenderTargets);
	
	int32 Width, Height, NumChannels;
	uint8* StallmanPixels = GPlatform->LoadImage("../Images/Stallman.jpg", Width, Height, NumChannels);
	GLImageRef Stallman = GLCreateImage(Width, Height, IF_R8G8B8A8_UNORM, RU_ShaderResource, StallmanPixels);
	GPlatform->FreeImage(StallmanPixels);

	GLShaderRef TestVert = GLCreateShader<TestVS>();
	GLShaderRef FullscreenVert = GLCreateShader<FullscreenVS>();
	GLShaderRef FullscreenFrag = GLCreateShader<FullscreenFS>();
	GLShaderRef RenderTargetFrag = GLCreateShader<RenderTargetFS>();

	std::array<glm::vec3, 4> VertexPositions =
	{
		glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f)
	};

	std::array<glm::vec2, 4> VertexUVs = 
	{
		glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f)
	};

	std::array<uint32, 6> Indices =
	{
		0, 1, 2, 1, 3, 2
	};

	GLIndexBufferRef IndexBuffer = GLCreateIndexBuffer(IF_R32_UINT, Indices.size(), RU_None, Indices.data());
	GLVertexBufferRef PositionVertexBuffer = GLCreateVertexBuffer(IF_R32G32B32_SFLOAT, VertexPositions.size(), RU_None, VertexPositions.data());
	GLVertexBufferRef TextureCoordinateVertexBuffer = GLCreateVertexBuffer(IF_R32G32_SFLOAT, VertexUVs.size(), RU_None, VertexUVs.data());

	GLImageRef Depth = GLCreateImage(GPlatform->GetWindowSize().x, GPlatform->GetWindowSize().y, IF_D32_SFLOAT, RU_RenderTargetable);
	GLRenderTargetViewRef DepthView = GLCreateRenderTargetView(Depth, ELoadAction::Clear, EStoreAction::Store, 1.0f, 0);
	
	std::array<float, 4> ClearColor = { 0, 0, 0, 0 };

	GLImageRef RenderTarget = GLCreateImage(GPlatform->GetWindowSize().x, GPlatform->GetWindowSize().y,
		IF_R8G8B8A8_SRGB, RU_RenderTargetable | RU_ShaderResource);
	GLRenderTargetViewRef RenderTargetView = GLCreateRenderTargetView(RenderTarget, ELoadAction::Clear, EStoreAction::Store, ClearColor);

	while (!GPlatform->WindowShouldClose())
	{
		GPlatform->PollEvents();

		GLBeginRender();

		/** Render to image */
		GLSetRenderTargets(1, &RenderTargetView, nullptr, DS_None);
		GLSetViewport(0.0f, 0.0f, (float)GPlatform->GetWindowSize().x, (float)GPlatform->GetWindowSize().y);
		GLSetDepthTest(false);
		GLSetColorMask(0, Color_RGBA);
		GLSetRasterizerState(CM_None);
		GLSetGraphicsPipeline(
			TestVert,
			nullptr,
			nullptr,
			nullptr,
			RenderTargetFrag);
		GLSetShaderImage(RenderTargetFrag, 0, Stallman, SamplerState());
		GLSetVertexStream(0, PositionVertexBuffer);
		GLSetVertexStream(1, TextureCoordinateVertexBuffer);
		GLDrawIndexed(IndexBuffer, Indices.size(), 1, 0, 0, 0);

		/** Sample the image */
		GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Clear, EStoreAction::Store, ClearColor);
		GLSetRenderTargets(1, &SurfaceView, nullptr, DS_None);
		GLSetGraphicsPipeline(
			TestVert,
			nullptr,
			nullptr,
			nullptr,
			FullscreenFrag);
		GLSetShaderImage(FullscreenFrag, 0, RenderTarget, SamplerState());
		GLSetVertexStream(0, PositionVertexBuffer);
		GLSetVertexStream(1, TextureCoordinateVertexBuffer);
		GLDrawIndexed(IndexBuffer, Indices.size(), 1, 0, 0, 0);

		GLEndRender();
	}
}

int main(int argc, char* argv[])
{
	cxxopts::Options Options("VulkanGL", "A Vulkan-based framework for graphics demos :)");

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
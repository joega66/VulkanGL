#include "SceneRenderer.h"
#include <Engine/Screen.h>

class SkyboxVS : public drm::Shader
{
public:
	SkyboxVS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		SceneProxy::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

class SkyboxFS : public drm::Shader
{
public:
	SkyboxFS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
		CompilationInfo.Bind("Skybox", Skybox);
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		SceneProxy::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

	ShaderBinding Skybox;
};

void SceneRenderer::RenderSkybox(SceneProxy& Scene, RenderCommandList& CmdList)
{
	Ref<SkyboxVS> VertShader = *ShaderMapRef<SkyboxVS>();
	Ref<SkyboxFS> FragShader = *ShaderMapRef<SkyboxFS>();

	drm::DescriptorSetRef DescriptorSet = drm::CreateDescriptorSet();

	DescriptorSet->Write(Scene.Skybox, SamplerState{ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }, FragShader->Skybox);
	DescriptorSet->Update();

	std::array<drm::DescriptorSetRef, 2> DescriptorSets =
	{
		Scene.DescriptorSet,
		DescriptorSet
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());

	PipelineStateInitializer PSOInit = {};
	PSOInit.Viewport.Width = gScreen.GetWidth();
	PSOInit.Viewport.Height = gScreen.GetHeight();
	PSOInit.GraphicsPipelineState = { VertShader, nullptr, nullptr, nullptr, FragShader };

	CmdList.BindPipeline(PSOInit);

	for (const auto& Element : Cube->Batch.Elements)
	{
		CmdList.BindVertexBuffers(1, &Element.GetPositionBuffer());
		CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
	}
}
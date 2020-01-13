#include "SceneRenderer.h"

class SkyboxVS : public drm::Shader
{
public:
	SkyboxVS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
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
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

void SceneRenderer::RenderSkybox(SceneProxy& Scene, drm::CommandList& CmdList)
{
	Ref<SkyboxVS> VertShader = Scene.ShaderMap.FindShader<SkyboxVS>();
	Ref<SkyboxFS> FragShader = Scene.ShaderMap.FindShader<SkyboxFS>();
	
	drm::DescriptorSetRef DescriptorSet = Device.CreateDescriptorSet();

	DescriptorSet->Write(Scene.Skybox, SamplerState{ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }, 0);
	DescriptorSet->Update();

	std::array<drm::DescriptorSetRef, 2> DescriptorSets =
	{
		Scene.DescriptorSet,
		DescriptorSet
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());

	PipelineStateInitializer PSOInit = {};
	PSOInit.Viewport.Width = Scene.GetWidth();
	PSOInit.Viewport.Height = Scene.GetHeight();
	PSOInit.ShaderStages = { VertShader, nullptr, nullptr, nullptr, FragShader };

	CmdList.BindPipeline(PSOInit);

	for (const auto& Submesh : Cube->Submeshes)
	{
		CmdList.BindVertexBuffers(1, &Submesh.GetPositionBuffer());
		CmdList.DrawIndexed(Submesh.GetIndexBuffer(), Submesh.GetIndexCount(), 1, 0, 0, 0);
	}
}
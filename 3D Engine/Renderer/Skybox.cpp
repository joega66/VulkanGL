#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/AssetManager.h>
#include "GlobalRenderResources.h"

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
	auto& GlobalResources = ECS.GetSingletonComponent<GlobalRenderResources>();

	const SkyboxVS* VertShader = ShaderMap.FindShader<SkyboxVS>();
	const SkyboxFS* FragShader = ShaderMap.FindShader<SkyboxFS>();

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = GlobalResources.LightingRP;
	PSODesc.Viewport.Width = GlobalResources.SceneColor.GetWidth();
	PSODesc.Viewport.Height = GlobalResources.SceneColor.GetHeight();
	PSODesc.ShaderStages = { VertShader, nullptr, nullptr, nullptr, FragShader };
	PSODesc.DescriptorSets = { GlobalResources.CameraDescriptorSet, GlobalResources.SkyboxDescriptorSet };

	drm::Pipeline Pipeline = Device.CreatePipeline(PSODesc);

	CmdList.BindPipeline(Pipeline);

	CmdList.BindDescriptorSets(Pipeline, PSODesc.DescriptorSets.size(), PSODesc.DescriptorSets.data());

	const StaticMesh* Cube = Assets.GetStaticMesh("Cube");
	for (const auto& Submesh : Cube->Submeshes)
	{
		CmdList.BindVertexBuffers(1, &Submesh.GetPositionBuffer());
		CmdList.DrawIndexed(Submesh.GetIndexBuffer(), Submesh.GetIndexCount(), 1, 0, 0, 0);
	}
}
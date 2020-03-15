#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/AssetManager.h>
#include "GlobalRenderData.h"

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
	auto& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();

	const SkyboxVS* VertShader = ShaderMap.FindShader<SkyboxVS>();
	const SkyboxFS* FragShader = ShaderMap.FindShader<SkyboxFS>();

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = GlobalData.LightingRP;
	PSODesc.Viewport.Width = GlobalData.SceneColor.GetWidth();
	PSODesc.Viewport.Height = GlobalData.SceneColor.GetHeight();
	PSODesc.ShaderStages = { VertShader, nullptr, nullptr, nullptr, FragShader };
	PSODesc.Layouts = { GlobalData.CameraDescriptorSet.GetLayout(), GlobalData.SkyboxDescriptorSet.GetLayout() };

	std::shared_ptr<drm::Pipeline> Pipeline = Device.CreatePipeline(PSODesc);

	CmdList.BindPipeline(Pipeline);

	const std::vector<VkDescriptorSet> DescriptorSets = { GlobalData.CameraDescriptorSet, GlobalData.SkyboxDescriptorSet };
	CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

	const StaticMesh* Cube = Assets.GetStaticMesh("Cube");
	for (const auto& Submesh : Cube->Submeshes)
	{
		CmdList.BindVertexBuffers(1, &Submesh.GetPositionBuffer());
		CmdList.DrawIndexed(Submesh.GetIndexBuffer(), Submesh.GetIndexCount(), 1, 0, 0, 0);
	}
}
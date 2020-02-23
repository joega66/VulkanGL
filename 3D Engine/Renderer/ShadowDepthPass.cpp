#include "MaterialShader.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include "FullscreenQuad.h"
#include <ECS/EntityManager.h>

void SceneRenderer::RenderShadowDepths(SceneProxy& Scene, drm::CommandList& CmdList)
{
	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		ShadowProxy& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);
		ShadowProxy.Render(CmdList);
	}
}

class ShadowProjectionFS : public drm::Shader
{
public:
	ShadowProjectionFS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/ShadowProjectionFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneRenderer::RenderShadowMask(SceneProxy& Scene, drm::CommandList& CmdList)
{
	CmdList.BeginRenderPass(ShadowMaskRP);

	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		const ShadowProxy& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);

		PipelineStateDesc PSODesc = {};
		PSODesc.RenderPass = &ShadowMaskRP;
		PSODesc.Viewport.Width = ShadowMask.GetWidth();
		PSODesc.Viewport.Height = ShadowMask.GetHeight();
		PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSODesc.DepthStencilState.DepthWriteEnable = false;
		PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<FullscreenVS>();
		PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<ShadowProjectionFS>();
		PSODesc.DescriptorSets = {
			CameraDescriptorSet,
			SceneTexturesDescriptorSet,
			&ShadowProxy.GetDescriptorSet()
		};

		drm::Pipeline Pipeline = Device.CreatePipeline(PSODesc);

		CmdList.BindPipeline(Pipeline);

		CmdList.BindDescriptorSets(Pipeline, PSODesc.DescriptorSets.size(), PSODesc.DescriptorSets.data());

		CmdList.Draw(3, 1, 0, 0);
	}

	CmdList.EndRenderPass();
}
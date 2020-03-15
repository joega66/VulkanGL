#include "MaterialShader.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include "FullscreenQuad.h"
#include "GlobalRenderData.h"
#include <ECS/EntityManager.h>

void SceneRenderer::RenderShadowDepths(SceneProxy& Scene, drm::CommandList& CmdList)
{
	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		ShadowProxy& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);
		ShadowProxy.Render(CmdList);
	}
}

void SceneRenderer::RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	auto& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();

	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		const auto& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);

		PipelineStateDesc PSODesc = {};
		PSODesc.RenderPass = GlobalData.DepthVisualizationRP;
		PSODesc.Viewport.Width = GlobalData.SceneDepth.GetWidth();
		PSODesc.Viewport.Height = GlobalData.SceneDepth.GetHeight();
		PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSODesc.DepthStencilState.DepthWriteEnable = false;
		PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<FullscreenVS>();
		PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<FullscreenFS<EVisualize::Depth>>();
		PSODesc.Layouts = { ShadowProxy.GetDescriptorSet().GetLayout() };

		std::shared_ptr<drm::Pipeline> DepthVisualizationPipeline = Device.CreatePipeline(PSODesc);

		CmdList.BeginRenderPass(GlobalData.DepthVisualizationRP);

		CmdList.BindPipeline(DepthVisualizationPipeline);

		const std::vector<VkDescriptorSet> DescriptorSets = {  };
		CmdList.BindDescriptorSets(DepthVisualizationPipeline, 1, &ShadowProxy.GetDescriptorSet().GetNativeHandle());

		CmdList.Draw(3, 1, 0, 0);

		CmdList.EndRenderPass();
	}
}
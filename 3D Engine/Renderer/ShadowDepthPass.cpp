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

void SceneRenderer::RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		const auto& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);

		PipelineStateDesc PSODesc = {};
		PSODesc.RenderPass = DepthVisualizationRP;
		PSODesc.Viewport.Width = SceneDepth.GetWidth();
		PSODesc.Viewport.Height = SceneDepth.GetHeight();
		PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSODesc.DepthStencilState.DepthWriteEnable = false;
		PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<FullscreenVS>();
		PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<FullscreenFS<EVisualize::Depth>>();
		PSODesc.DescriptorSets = { &ShadowProxy.GetDescriptorSet() };

		drm::Pipeline DepthVisualizationPipeline = Device.CreatePipeline(PSODesc);

		CmdList.BeginRenderPass(DepthVisualizationRP);

		CmdList.BindPipeline(DepthVisualizationPipeline);

		CmdList.BindDescriptorSets(DepthVisualizationPipeline, PSODesc.DescriptorSets.size(), PSODesc.DescriptorSets.data());

		CmdList.Draw(3, 1, 0, 0);

		CmdList.EndRenderPass();
	}
}
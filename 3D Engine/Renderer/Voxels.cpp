#include "SceneRenderer.h"

static const glm::uvec2 VoxelGridSize = { 512, 512 };



void SceneRenderer::RenderVoxels(SceneProxy& Scene, RenderCommandList& CmdList)
{
	RenderPassInitializer RPInit = { 0 }; // Disable ROP
	RPInit.RenderArea.Extent = VoxelGridSize;

	CmdList.BeginRenderPass(RPInit);

	PipelineStateInitializer PSOInit;
	PSOInit.DepthStencilState.DepthTestEnable = false;
	PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
	PSOInit.Viewport.Width = VoxelGridSize.x;
	PSOInit.Viewport.Height = VoxelGridSize.y;

	CmdList.EndRenderPass();
}
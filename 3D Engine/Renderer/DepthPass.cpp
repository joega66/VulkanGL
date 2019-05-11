#include "DepthPass.h"
#include <Engine/StaticMesh.h>

DepthPassDrawingPlan::DepthPassDrawingPlan(const StaticMeshResources& Resources, drm::UniformBufferRef InLocalToWorldUniform)
{
	LocalToWorldUniform = InLocalToWorldUniform;

	IndexCount = Resources.IndexCount;
	IndexBuffer = Resources.IndexBuffer;

	VertexShader = *ShaderMapRef<DepthPassVS<EMeshType::StaticMesh>>();

	PositionStream = { Resources.PositionBuffer, 0 };
}

GraphicsPipelineState DepthPassDrawingPlan::GetGraphicsPipeline() const
{
	return GraphicsPipelineState{
		VertexShader
		, nullptr
		, nullptr
		, nullptr
		, nullptr };
}

void DepthPassDrawingPlan::SetUniforms(RenderCommandList& CmdList, const View& View)
{
	VertexShader->SetUniforms(CmdList, View.Uniform, LocalToWorldUniform);
}

void DepthPassDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	CmdList.SetVertexStream(PositionStream.Location, PositionStream.VertexBuffer);
	CmdList.DrawIndexed(IndexBuffer, IndexCount, 1, 0, 0, 0);
}
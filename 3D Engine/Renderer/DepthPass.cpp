#include "DepthPass.h"
#include <Engine/StaticMesh.h>

DepthPassDrawingPlan::DepthPassDrawingPlan(const MeshElement& Element, drm::UniformBufferRef InLocalToWorldUniform)
	: Element(Element)
{
	LocalToWorldUniform = InLocalToWorldUniform;
	VertexShader = *ShaderMapRef<DepthPassVS<EMeshType::StaticMesh>>();
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
	CmdList.BindVertexBuffers(0, Element.PositionBuffer);
	CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
}
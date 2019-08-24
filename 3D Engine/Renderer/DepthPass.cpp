#include "DepthPass.h"
#include <Engine/StaticMesh.h>
#include "Scene.h"

DepthPassDrawingPlan::DepthPassDrawingPlan(const MeshElement& Element, drm::UniformBufferRef InLocalToWorldUniform)
	: Element(Element)
{
	LocalToWorldUniform = InLocalToWorldUniform;
	VertexShader = *ShaderMapRef<DepthPassVS<EMeshType::StaticMesh>>();
}

void DepthPassDrawingPlan::SetPipelineState(PipelineStateInitializer& PSOInit) const
{
	PSOInit.GraphicsPipelineState =
	{
		VertexShader,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};
}

void DepthPassDrawingPlan::SetUniforms(RenderCommandList& CmdList, const Scene& Scene)
{
	Scene.SetResources(CmdList, VertexShader, VertexShader->SceneBindings);
	CmdList.SetUniformBuffer(VertexShader, VertexShader->LocalToWorld, LocalToWorldUniform);
}

void DepthPassDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	CmdList.BindVertexBuffers(1, &Element.GetPositionBuffer());
	CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
}
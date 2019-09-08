#include "DepthPass.h"
#include <Engine/StaticMesh.h>
#include "SceneRenderer.h"

DepthPassDrawPlan::DepthPassDrawPlan(const MeshElement& Element, drm::UniformBufferRef InLocalToWorldUniform)
	: Element(Element)
{
	LocalToWorldUniform = InLocalToWorldUniform;
	VertexShader = *ShaderMapRef<DepthPassVS<EMeshType::StaticMesh>>();
}

void DepthPassDrawPlan::SetPipelineState(PipelineStateInitializer& PSOInit) const
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

void DepthPassDrawPlan::SetUniforms(RenderCommandList& CmdList, const SceneProxy& Scene)
{
	Scene.SetResources(CmdList, VertexShader, VertexShader->SceneBindings);
	CmdList.SetUniformBuffer(VertexShader, VertexShader->LocalToWorld, LocalToWorldUniform);
}

void DepthPassDrawPlan::Draw(RenderCommandList& CmdList) const
{
	CmdList.BindVertexBuffers(1, &Element.GetPositionBuffer());
	CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
}
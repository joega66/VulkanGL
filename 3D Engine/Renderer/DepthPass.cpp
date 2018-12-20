#include "DepthPass.h"
#include "Engine/StaticMesh.h"

DepthPassDrawingPlan::DepthPassDrawingPlan(const StaticMeshResources& Resources, GLUniformBufferRef InLocalToWorldUniform)
{
	LocalToWorldUniform = InLocalToWorldUniform;

	IndexCount = Resources.IndexCount;
	IndexBuffer = Resources.IndexBuffer;

	VertexShader = GLCreateShader<DepthPassVS<EMeshType::StaticMesh>>();

	PositionStream = { Resources.PositionBuffer, VertexShader->GetAttributeLocation("Position") };

	ViewLocation = VertexShader->GetUniformLocation("ViewUniform");
	LocalToWorldLocation = VertexShader->GetUniformLocation("LocalToWorldUniform");
}

GraphicsPipeline DepthPassDrawingPlan::GetGraphicsPipeline() const
{
	return GraphicsPipeline(
		VertexShader
		, nullptr
		, nullptr
		, nullptr
		, nullptr);
}

void DepthPassDrawingPlan::SetUniforms(const View& View)
{
	GLSetUniformBuffer(VertexShader, ViewLocation, View.Uniform);
	GLSetUniformBuffer(VertexShader, LocalToWorldLocation, LocalToWorldUniform);
}

void DepthPassDrawingPlan::Draw() const
{
	GLSetVertexStream(PositionStream.Location, PositionStream.VertexBuffer);
	GLDrawIndexed(IndexBuffer, IndexCount, 1, 0, 0, 0);
}
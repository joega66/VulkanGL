#include "EditorPrimitives.h"
#include "Engine/StaticMesh.h"

OutlineDrawingPlan::OutlineDrawingPlan(const StaticMeshResources& Resources, GLUniformBufferRef LocalToWorldUniform)
	: DepthPassDrawingPlan(Resources, LocalToWorldUniform)
{
	FragmentShader = GLCreateShader<OutlineFS>();
}

GraphicsPipeline OutlineDrawingPlan::GetGraphicsPipeline() const
{
	GraphicsPipeline Pipeline = DepthPassDrawingPlan::GetGraphicsPipeline();

	return GraphicsPipeline(
	Pipeline.Vertex
	, nullptr
	, nullptr
	, nullptr
	, FragmentShader);
}

void OutlineDrawingPlan::SetUniforms(const View& View)
{
	DepthPassDrawingPlan::SetUniforms(View);
}

void OutlineDrawingPlan::Draw() const
{
	DepthPassDrawingPlan::Draw();
}
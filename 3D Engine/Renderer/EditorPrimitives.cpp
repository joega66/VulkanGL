#include "EditorPrimitives.h"
#include "Engine/StaticMesh.h"

ObjectHighlightDrawingPlan::ObjectHighlightDrawingPlan(const StaticMeshResources& Resources, GLUniformBufferRef LocalToWorldUniform)
	: DepthPassDrawingPlan(Resources, LocalToWorldUniform)
{
	FragmentShader = GLCreateShader<ObjectHighlightFS>();
}

GraphicsPipeline ObjectHighlightDrawingPlan::GetGraphicsPipeline() const
{
	GraphicsPipeline Pipeline = DepthPassDrawingPlan::GetGraphicsPipeline();

	return GraphicsPipeline(
	Pipeline.Vertex
	, nullptr
	, nullptr
	, nullptr
	, FragmentShader);
}

void ObjectHighlightDrawingPlan::SetUniforms(const View& View)
{
	DepthPassDrawingPlan::SetUniforms(View);
}

void ObjectHighlightDrawingPlan::Draw() const
{
	DepthPassDrawingPlan::Draw();
}
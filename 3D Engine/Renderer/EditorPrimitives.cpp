#include "EditorPrimitives.h"
#include <Engine/StaticMesh.h>

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

LineDrawingPlan::LineDrawingPlan(const glm::vec3 & A, const glm::vec3 & B, const glm::vec4 & Color, float Width)
	: LineWidth(Width)
{
	std::vector<glm::vec3> Positions = 
	{
		A, B, B
	};

	PositionBuffer = GLCreateVertexBuffer(IF_R32G32B32_SFLOAT, Positions.size(), EResourceUsage::None, Positions.data());
	ColorUniform = GLCreateUniformBuffer(Color, EUniformUpdate::SingleFrame);
}

void LineDrawingPlan::GetPipelineState(PipelineStateInitializer& PSOInit) const
{
	PSOInit.RasterizationState.LineWidth = LineWidth;
}

GraphicsPipeline LineDrawingPlan::GetGraphicsPipeline() const
{
	return GraphicsPipeline(
		GLCreateShader<LinesVS>(),
		nullptr,
		nullptr,
		nullptr,
		GLCreateShader<LinesFS>()
	);
}

void LineDrawingPlan::SetUniforms(const View& View)
{
	GLShaderRef VertShader = GLCreateShader<LinesVS>();
	GLShaderRef FragShader = GLCreateShader<LinesFS>();

	GLSetUniformBuffer(VertShader, "ViewUniform", View.Uniform);
	GLSetUniformBuffer(FragShader, "ColorUniform", ColorUniform);
}

void LineDrawingPlan::Draw() const
{
	GLShaderRef VertShader = GLCreateShader<LinesVS>();

	GLSetVertexStream(VertShader->GetAttributeLocation("Position"), PositionBuffer);
	GLDraw(3, 1, 0, 0);
}
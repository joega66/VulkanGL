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

void OutlineDrawingPlan::SetUniforms(RenderCommandList& CmdList, const View& View)
{
	DepthPassDrawingPlan::SetUniforms(CmdList, View);
}

void OutlineDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	DepthPassDrawingPlan::Draw(CmdList);
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

void LineDrawingPlan::SetUniforms(RenderCommandList& CmdList, const View& View)
{
	GLShaderRef VertShader = GLCreateShader<LinesVS>();
	GLShaderRef FragShader = GLCreateShader<LinesFS>();

	CmdList.SetUniformBuffer(VertShader, VertShader->GetUniformLocation("ViewUniform"), View.Uniform);
	CmdList.SetUniformBuffer(FragShader, FragShader->GetUniformLocation("ColorUniform"), ColorUniform);
}

void LineDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	GLShaderRef VertShader = GLCreateShader<LinesVS>();

	CmdList.SetVertexStream(VertShader->GetAttributeLocation("Position"), PositionBuffer);
	CmdList.Draw(3, 1, 0, 0);
}
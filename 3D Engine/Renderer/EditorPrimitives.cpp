#include "EditorPrimitives.h"
#include <Engine/StaticMesh.h>

OutlineDrawingPlan::OutlineDrawingPlan(const StaticMeshResources& Resources, drm::UniformBufferRef LocalToWorldUniform)
	: DepthPassDrawingPlan(Resources, LocalToWorldUniform)
{
	FragmentShader = *ShaderMapRef<OutlineFS>();
}

GraphicsPipelineState OutlineDrawingPlan::GetGraphicsPipeline() const
{
	GraphicsPipelineState Pipeline = DepthPassDrawingPlan::GetGraphicsPipeline();

	return GraphicsPipelineState{
	Pipeline.Vertex
	, nullptr
	, nullptr
	, nullptr
	, FragmentShader
	};
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

	PositionBuffer = drm::CreateVertexBuffer(EImageFormat::R32G32B32_SFLOAT, Positions.size(), EResourceUsage::None, Positions.data());
	ColorUniform = drm::CreateUniformBuffer(Color, EUniformUpdate::SingleFrame);
}

void LineDrawingPlan::GetPipelineState(PipelineStateInitializer& PSOInit) const
{
	PSOInit.RasterizationState.LineWidth = LineWidth;
}

GraphicsPipelineState LineDrawingPlan::GetGraphicsPipeline() const
{
	return GraphicsPipelineState{
		*ShaderMapRef<LinesVS>(),
		nullptr,
		nullptr,
		nullptr,
		*ShaderMapRef<LinesFS>()
	};
}

void LineDrawingPlan::SetUniforms(RenderCommandList& CmdList, const View& View)
{
	Ref<LinesVS> VertShader = *ShaderMapRef<LinesVS>();
	Ref<LinesFS> FragShader = *ShaderMapRef<LinesFS>();

	VertShader->SetUniforms(CmdList, View.Uniform);
	FragShader->SetUniforms(CmdList, ColorUniform);
}

void LineDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	CmdList.SetVertexStream(0, PositionBuffer);
	CmdList.Draw(3, 1, 0, 0);
}
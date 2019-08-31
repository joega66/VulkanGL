#include "EditorPrimitives.h"
#include <Engine/StaticMesh.h>
#include "SceneRenderer.h"

OutlineDrawingPlan::OutlineDrawingPlan(const MeshElement& Element, drm::UniformBufferRef LocalToWorldUniform)
	: DepthPassDrawingPlan(Element, LocalToWorldUniform)
{
	FragmentShader = *ShaderMapRef<OutlineFS>();
}

void OutlineDrawingPlan::SetPipelineState(PipelineStateInitializer& PSOInit) const
{
	PSOInit.GraphicsPipelineState =
	{
		VertexShader,
		nullptr,
		nullptr,
		nullptr,
		FragmentShader
	};
}

void OutlineDrawingPlan::SetUniforms(RenderCommandList& CmdList, const SceneRenderer& SceneRenderer)
{
	DepthPassDrawingPlan::SetUniforms(CmdList, SceneRenderer);
}

void OutlineDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	DepthPassDrawingPlan::Draw(CmdList);
}

LineDrawingPlan::LineDrawingPlan(const glm::vec3& A, const glm::vec3& B, const glm::vec4& Color, float Width)
	: LineWidth(Width)
{
	std::vector<glm::vec3> Positions =
	{
		A, B, B
	};

	PositionBuffer = drm::CreateVertexBuffer(EImageFormat::R32G32B32_SFLOAT, Positions.size(), EResourceUsage::None, Positions.data());
	ColorUniform = drm::CreateUniformBuffer(sizeof(Color), &Color, EUniformUpdate::SingleFrame);
}

void LineDrawingPlan::SetPipelineState(PipelineStateInitializer& PSOInit) const
{
	PSOInit.GraphicsPipelineState =
	{
		*ShaderMapRef<LinesVS>(),
		nullptr,
		nullptr,
		nullptr,
		*ShaderMapRef<LinesFS>()
	};

	PSOInit.RasterizationState.LineWidth = LineWidth;
}

void LineDrawingPlan::SetUniforms(RenderCommandList& CmdList, const SceneRenderer& SceneRenderer)
{
	Ref<LinesVS> VertShader = *ShaderMapRef<LinesVS>();
	Ref<LinesFS> FragShader = *ShaderMapRef<LinesFS>();

	SceneRenderer.SetResources(CmdList, VertShader, VertShader->SceneBindings);
	FragShader->SetUniforms(CmdList, ColorUniform);
}

void LineDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	CmdList.BindVertexBuffers(1, &PositionBuffer);
	CmdList.Draw(3, 1, 0, 0);
}
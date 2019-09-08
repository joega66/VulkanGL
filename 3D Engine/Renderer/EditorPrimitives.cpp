#include "EditorPrimitives.h"
#include <Engine/StaticMesh.h>
#include "SceneRenderer.h"

OutlineDrawPlan::OutlineDrawPlan(const MeshElement& Element, drm::UniformBufferRef LocalToWorldUniform)
	: DepthPassDrawPlan(Element, LocalToWorldUniform)
{
	FragmentShader = *ShaderMapRef<OutlineFS>();
}

void OutlineDrawPlan::SetPipelineState(PipelineStateInitializer& PSOInit) const
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

void OutlineDrawPlan::SetUniforms(RenderCommandList& CmdList, const SceneProxy& Scene)
{
	DepthPassDrawPlan::SetUniforms(CmdList, Scene);
}

void OutlineDrawPlan::Draw(RenderCommandList& CmdList) const
{
	DepthPassDrawPlan::Draw(CmdList);
}

LineDrawPlan::LineDrawPlan(const glm::vec3& A, const glm::vec3& B, const glm::vec4& Color, float Width)
	: LineWidth(Width)
{
	std::vector<glm::vec3> Positions =
	{
		A, B, B
	};

	PositionBuffer = drm::CreateVertexBuffer(EImageFormat::R32G32B32_SFLOAT, Positions.size(), EResourceUsage::None, Positions.data());
	ColorUniform = drm::CreateUniformBuffer(sizeof(Color), &Color, EUniformUpdate::SingleFrame);
}

void LineDrawPlan::SetPipelineState(PipelineStateInitializer& PSOInit) const
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

void LineDrawPlan::SetUniforms(RenderCommandList& CmdList, const SceneProxy& Scene)
{
	Ref<LinesVS> VertShader = *ShaderMapRef<LinesVS>();
	Ref<LinesFS> FragShader = *ShaderMapRef<LinesFS>();

	Scene.SetResources(CmdList, VertShader, VertShader->SceneBindings);
	FragShader->SetUniforms(CmdList, ColorUniform);
}

void LineDrawPlan::Draw(RenderCommandList& CmdList) const
{
	CmdList.BindVertexBuffers(1, &PositionBuffer);
	CmdList.Draw(3, 1, 0, 0);
}
#include "LightingPass.h"
#include <Renderer/Scene.h>
#include <GL.h>

LightingPassDrawingPlan::LightingPassDrawingPlan(const StaticMeshResources& Resources, CMaterial& CMaterial, GLUniformBufferRef LocalToWorldUniform)
{
	const bool bHasDiffuseMap = std::holds_alternative<GLImageRef>(CMaterial.Diffuse);
	const bool bHasNormalMap = CMaterial.Normal != nullptr;

	LightingPassVert = GLCreateShader<LightingPassVS<EMeshType::StaticMesh>>();

	// @todo This is already becoming unwieldy...
	if (bHasDiffuseMap && bHasNormalMap)
	{
		LightingPassFrag = GLCreateShader<LightingPassFS<true, true, EMeshType::StaticMesh>>();
	}
	else if (bHasDiffuseMap && !bHasNormalMap)
	{
		LightingPassFrag = GLCreateShader<LightingPassFS<true, false, EMeshType::StaticMesh>>();
	}
	else if (!bHasDiffuseMap && bHasNormalMap)
	{
		LightingPassFrag = GLCreateShader<LightingPassFS<false, true, EMeshType::StaticMesh>>();
	}
	else
	{
		LightingPassFrag = GLCreateShader<LightingPassFS<false, false, EMeshType::StaticMesh>>();
	}

	MaterialDrawingPlan::Construct(Resources, CMaterial, LocalToWorldUniform, GetGraphicsPipeline());
}

GraphicsPipeline LightingPassDrawingPlan::GetGraphicsPipeline() const
{
	return GraphicsPipeline(
		LightingPassVert,
		nullptr,
		nullptr,
		nullptr,
		LightingPassFrag
	);
}

void LightingPassDrawingPlan::SetUniforms(const View& View)
{
	MaterialDrawingPlan::SetUniforms(View, GetGraphicsPipeline());
}

void LightingPassDrawingPlan::Draw() const
{
	MaterialDrawingPlan::Draw();
}
#include "LightingPass.h"
#include "../GL.h"

// @todo Since we're querying attribute/uniform locations anyway, should use glsl compiler option to automatically assign locations?

LightingPassDrawingPlan::LightingPassDrawingPlan(const StaticMeshResources& Resources, MaterialProxyRef MaterialProxy, GLUniformBufferRef LocalToWorldUniform)
{
	const bool bHasNormalMap = MaterialProxy->Count(EMaterialType::Normal);

	LightingPassVert = GLCreateShader<LightingPassVS<EMeshType::StaticMesh>>();

	if (bHasNormalMap)
	{
		LightingPassFrag = GLCreateShader<LightingPassFS<true, EMeshType::StaticMesh>>();
	}
	else
	{
		LightingPassFrag = GLCreateShader<LightingPassFS<false, EMeshType::StaticMesh>>();
	}

	MaterialDrawingPlan::Construct(Resources, MaterialProxy, LocalToWorldUniform, GetGraphicsPipeline());
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
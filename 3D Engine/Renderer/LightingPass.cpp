#include "LightingPass.h"
#include <Renderer/Scene.h>
#include <DRM.h>

LightingPassDrawingPlan::LightingPassDrawingPlan(const StaticMeshResources& Resources, CMaterial& CMaterial, drm::UniformBufferRef LocalToWorldUniform)
{
	const bool bHasDiffuseMap = std::holds_alternative<drm::ImageRef>(CMaterial.Diffuse);
	const bool bHasNormalMap = CMaterial.Normal != nullptr;

	LightingPassVert = drm::CreateShader<LightingPassVS<EMeshType::StaticMesh>>();

	if (bHasDiffuseMap && bHasNormalMap)
	{
		LightingPassFrag = drm::CreateShader<LightingPassFS<true, true, EMeshType::StaticMesh>>();
	}
	else if (bHasDiffuseMap && !bHasNormalMap)
	{
		LightingPassFrag = drm::CreateShader<LightingPassFS<true, false, EMeshType::StaticMesh>>();
	}
	else if (!bHasDiffuseMap && bHasNormalMap)
	{
		LightingPassFrag = drm::CreateShader<LightingPassFS<false, true, EMeshType::StaticMesh>>();
	}
	else
	{
		LightingPassFrag = drm::CreateShader<LightingPassFS<false, false, EMeshType::StaticMesh>>();
	}

	MaterialDrawingPlan::Construct(Resources, CMaterial, LocalToWorldUniform, GetGraphicsPipeline());
}

GraphicsPipelineState LightingPassDrawingPlan::GetGraphicsPipeline() const
{
	return GraphicsPipelineState{
		LightingPassVert,
		nullptr,
		nullptr,
		nullptr,
		LightingPassFrag
	};
}

void LightingPassDrawingPlan::SetUniforms(RenderCommandList& CmdList, const View& View)
{
	MaterialDrawingPlan::SetUniforms(CmdList, View, GetGraphicsPipeline());
}

void LightingPassDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	MaterialDrawingPlan::Draw(CmdList);
}
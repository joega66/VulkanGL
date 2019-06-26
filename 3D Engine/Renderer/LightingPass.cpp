#include "LightingPass.h"
#include <Renderer/Scene.h>
#include <DRM.h>

LightingPassDrawingPlan::LightingPassDrawingPlan(const struct MeshElement& Element, CMaterial& CMaterial, drm::UniformBufferRef LocalToWorldUniform)
	: Element(Element)
{
	const bool bHasDiffuseMap = std::holds_alternative<drm::ImageRef>(CMaterial.Diffuse);
	const bool bHasNormalMap = CMaterial.Normal != nullptr;

	VertShader = *ShaderMapRef<LightingPassVS<EMeshType::StaticMesh>>();

	// @todo Shader permutations? Or bypass the shader cache?
	if (bHasDiffuseMap && bHasNormalMap)
	{
		FragShader = *ShaderMapRef<LightingPassFS<true, true, EMeshType::StaticMesh>>();
	}
	else if (bHasDiffuseMap && !bHasNormalMap)
	{
		FragShader = *ShaderMapRef<LightingPassFS<true, false, EMeshType::StaticMesh>>();
	}
	else if (!bHasDiffuseMap && bHasNormalMap)
	{
		FragShader = *ShaderMapRef<LightingPassFS<false, true, EMeshType::StaticMesh>>();
	}
	else
	{
		FragShader = *ShaderMapRef<LightingPassFS<false, false, EMeshType::StaticMesh>>();
	}

	Uniforms.push_back({ VertShader, LocalToWorldUniform, VertShader->LocalToWorld });

	if (std::holds_alternative<drm::ImageRef>(CMaterial.Diffuse))
	{
		Materials.push_back({ std::get<drm::ImageRef>(CMaterial.Diffuse), FragShader->Diffuse });
	}
	else
	{
		Uniforms.push_back({ FragShader, drm::CreateUniformBuffer(std::get<glm::vec4>(CMaterial.Diffuse)), FragShader->DiffuseUniform });
	}

	if (bHasNormalMap)
	{
		Materials.push_back({ CMaterial.Normal, FragShader->Normal });
	}
}

GraphicsPipelineState LightingPassDrawingPlan::GetGraphicsPipeline() const
{
	return GraphicsPipelineState{
		VertShader,
		nullptr,
		nullptr,
		nullptr,
		FragShader
	};
}

void LightingPassDrawingPlan::SetUniforms(RenderCommandList& CmdList, const View& View)
{
	CmdList.SetUniformBuffer(VertShader, VertShader->View, View.Uniform);

	for (auto& Uniform : Uniforms)
	{
		CmdList.SetUniformBuffer(Uniform.Shader, Uniform.Location, Uniform.UniformBuffer);
	}

	for (auto& Material : Materials)
	{
		CmdList.SetShaderImage(FragShader, Material.Location, Material.Image, SamplerState{});
	}
}

void LightingPassDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	CmdList.BindVertexBuffers(0, Element.PositionBuffer);
	CmdList.BindVertexBuffers(1, Element.TextureCoordinateBuffer);
	CmdList.BindVertexBuffers(2, Element.NormalBuffer);
	CmdList.BindVertexBuffers(3, Element.TangentBuffer);
	CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
}
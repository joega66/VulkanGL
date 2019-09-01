#include "LightingPass.h"
#include "SceneRenderer.h"

LightingPassDrawPlan::LightingPassDrawPlan(const MeshElement& Element, CMaterial& CMaterial, drm::UniformBufferRef LocalToWorldUniform)
	: Element(Element)
{
	const bool bHasNormalMap = CMaterial.Normal != nullptr;

	VertShader = *ShaderMapRef<LightingPassVS<EMeshType::StaticMesh>>();

	// @todo Shader permutations? Or bypass the shader cache?
	if (bHasNormalMap)
	{
		FragShader = *ShaderMapRef<LightingPassFS<true, EMeshType::StaticMesh>>();
	}
	else 
	{
		FragShader = *ShaderMapRef<LightingPassFS<false, EMeshType::StaticMesh>>();
	}

	Uniforms.push_back({ VertShader, LocalToWorldUniform, VertShader->LocalToWorld });

	Materials.push_back({ CMaterial.Diffuse, FragShader->Diffuse });

	if (bHasNormalMap)
	{
		Materials.push_back({ CMaterial.Normal, FragShader->Normal });
	}
}

void LightingPassDrawPlan::SetPipelineState(PipelineStateInitializer& PSOInit) const
{
	PSOInit.GraphicsPipelineState = 
	{
		VertShader,
		nullptr,
		nullptr,
		nullptr,
		FragShader
	};
}

void LightingPassDrawPlan::SetUniforms(RenderCommandList& CmdList, const SceneRenderer& SceneRenderer)
{
	SceneRenderer.SetResources(CmdList, VertShader, VertShader->SceneBindings);
	SceneRenderer.SetResources(CmdList, FragShader, FragShader->SceneBindings);

	for (auto& Uniform : Uniforms)
	{
		CmdList.SetUniformBuffer(Uniform.Shader, Uniform.Location, Uniform.UniformBuffer);
	}

	for (auto& Material : Materials)
	{
		CmdList.SetShaderImage(FragShader, Material.Location, Material.Image, SamplerState{});
	}
}

void LightingPassDrawPlan::Draw(RenderCommandList& CmdList) const
{
	CmdList.BindVertexBuffers(Element.VertexBuffers.size(), Element.VertexBuffers.data());
	CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
}
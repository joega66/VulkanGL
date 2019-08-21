#include "LightingPass.h"
#include <Renderer/Scene.h>
#include <DRM.h>

LightingPassDrawingPlan::LightingPassDrawingPlan(const MeshElement& Element, CMaterial& CMaterial, drm::UniformBufferRef LocalToWorldUniform)
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

void LightingPassDrawingPlan::SetPipelineState(PipelineStateInitializer& PSOInit) const
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

void LightingPassDrawingPlan::SetUniforms(RenderCommandList& CmdList, const Scene& Scene)
{
	Scene.SetResources(CmdList, VertShader, VertShader->SceneBindings);
	Scene.SetResources(CmdList, FragShader, FragShader->SceneBindings);

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
	CmdList.BindVertexBuffers(Element.VertexBuffers.size(), Element.VertexBuffers.data());
	CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
}
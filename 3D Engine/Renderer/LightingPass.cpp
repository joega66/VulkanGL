#include "LightingPass.h"
#include <Renderer/Scene.h>
#include <DRM.h>

LightingPassDrawingPlan::LightingPassDrawingPlan(const StaticMeshResources& Resources, CMaterial& CMaterial, drm::UniformBufferRef LocalToWorldUniform)
{
	const bool bHasDiffuseMap = std::holds_alternative<drm::ImageRef>(CMaterial.Diffuse);
	const bool bHasNormalMap = CMaterial.Normal != nullptr;

	VertShader = *ShaderMapRef<LightingPassVS<EMeshType::StaticMesh>>();

	// @todo Shader permutations
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

	// @todo The index buffer and streams should be in static mesh since they won't change much?
	IndexCount = Resources.IndexCount;
	IndexBuffer = Resources.IndexBuffer;

	Streams.push_back({ Resources.PositionBuffer, 0 });
	Streams.push_back({ Resources.TextureCoordinateBuffer, 1 });
	Streams.push_back({ Resources.NormalBuffer, 2 });
	Streams.push_back({ Resources.TangentBuffer, 3 });

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
	for (const StreamSource& Stream : Streams)
	{
		CmdList.SetVertexStream(Stream.Location, Stream.VertexBuffer);
	}

	CmdList.DrawIndexed(IndexBuffer, IndexCount, 1, 0, 0, 0);
}
#include "MaterialShader.h"
#include <Engine/StaticMesh.h>

void MaterialDrawingPlan::Construct(const StaticMeshResources& Resources, 
	CMaterial& CMaterial,
	drm::UniformBufferRef LocalToWorldUniform, 
	GraphicsPipelineState&& Pipeline)
{
	IndexCount = Resources.IndexCount;
	IndexBuffer = Resources.IndexBuffer;

	Streams.push_back({ Resources.PositionBuffer, Pipeline.Vertex->GetAttributeLocation("Position") });
	Streams.push_back({ Resources.TextureCoordinateBuffer, Pipeline.Vertex->GetAttributeLocation("UV") });
	Streams.push_back({ Resources.NormalBuffer, Pipeline.Vertex->GetAttributeLocation("Normal") });
	Streams.push_back({ Resources.TangentBuffer, Pipeline.Vertex->GetAttributeLocation("Tangent") });

	ViewLocation = Pipeline.Vertex->GetUniformLocation("ViewUniform");

	Uniforms.push_back({ Pipeline.Vertex, LocalToWorldUniform, Pipeline.Vertex->GetUniformLocation("LocalToWorldUniform") });

	if (std::holds_alternative<drm::ImageRef>(CMaterial.Diffuse))
	{
		Materials.push_back({ std::get<drm::ImageRef>(CMaterial.Diffuse), Pipeline.Fragment->GetUniformLocation("Diffuse") });
	}
	else
	{
		Uniforms.push_back({ Pipeline.Fragment, drm::CreateUniformBuffer(std::get<glm::vec4>(CMaterial.Diffuse)), Pipeline.Fragment->GetUniformLocation("DiffuseUniform") });
	}

	if (CMaterial.Normal)
	{
		Materials.push_back({ CMaterial.Normal, Pipeline.Fragment->GetUniformLocation("Normal") });
	}
}

void MaterialDrawingPlan::SetUniforms(RenderCommandList& CmdList, const View& View, GraphicsPipelineState&& Pipeline)
{
	CmdList.SetUniformBuffer(Pipeline.Vertex, ViewLocation, View.Uniform);

	for (auto& Uniform : Uniforms)
	{
		CmdList.SetUniformBuffer(Uniform.Shader, Uniform.Location, Uniform.UniformBuffer);
	}

	for (auto& Material : Materials)
	{
		CmdList.SetShaderImage(Pipeline.Fragment, Material.Location, Material.Image, SamplerState{});
	}
}

void MaterialDrawingPlan::Draw(RenderCommandList& CmdList) const
{
	for (const StreamSource& Stream : Streams)
	{
		CmdList.SetVertexStream(Stream.Location, Stream.VertexBuffer);
	}

	CmdList.DrawIndexed(IndexBuffer, IndexCount, 1, 0, 0, 0);
}
#include "MaterialShader.h"
#include <Engine/StaticMesh.h>

void MaterialDrawingPlan::Construct(const StaticMeshResources& Resources, 
	CMaterial& CMaterial,
	GLUniformBufferRef LocalToWorldUniform, 
	GraphicsPipeline&& Pipeline)
{
	IndexCount = Resources.IndexCount;
	IndexBuffer = Resources.IndexBuffer;

	Streams.push_back({ Resources.PositionBuffer, Pipeline.Vertex->GetAttributeLocation("Position") });
	Streams.push_back({ Resources.TextureCoordinateBuffer, Pipeline.Vertex->GetAttributeLocation("UV") });
	Streams.push_back({ Resources.NormalBuffer, Pipeline.Vertex->GetAttributeLocation("Normal") });
	Streams.push_back({ Resources.TangentBuffer, Pipeline.Vertex->GetAttributeLocation("Tangent") });

	ViewLocation = Pipeline.Vertex->GetUniformLocation("ViewUniform");

	Uniforms.push_back({ Pipeline.Vertex, LocalToWorldUniform, Pipeline.Vertex->GetUniformLocation("LocalToWorldUniform") });

	if (std::holds_alternative<GLImageRef>(CMaterial.Diffuse))
	{
		Materials.push_back({ std::get<GLImageRef>(CMaterial.Diffuse), Pipeline.Fragment->GetUniformLocation("Diffuse") });
	}
	else
	{
		Uniforms.push_back({ Pipeline.Fragment, GLCreateUniformBuffer(std::get<glm::vec4>(CMaterial.Diffuse)), Pipeline.Fragment->GetUniformLocation("DiffuseUniform") });
	}

	if (CMaterial.Normal)
	{
		Materials.push_back({ CMaterial.Normal, Pipeline.Fragment->GetUniformLocation("Normal") });
	}
}

void MaterialDrawingPlan::SetUniforms(const View& View, GraphicsPipeline&& Pipeline)
{
	GLSetUniformBuffer(Pipeline.Vertex, ViewLocation, View.Uniform);

	for (auto& Uniform : Uniforms)
	{
		GLSetUniformBuffer(Uniform.Shader, Uniform.Location, Uniform.UniformBuffer);
	}

	for (auto& Material : Materials)
	{
		GLSetShaderImage(Pipeline.Fragment, Material.Location, Material.Image, SamplerState{});
	}
}

void MaterialDrawingPlan::Draw() const
{
	for (const StreamSource& Stream : Streams)
	{
		GLSetVertexStream(Stream.Location, Stream.VertexBuffer);
	}

	GLDrawIndexed(IndexBuffer, IndexCount, 1, 0, 0, 0);
}
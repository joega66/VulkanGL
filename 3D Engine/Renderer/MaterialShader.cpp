#include "MaterialShader.h"
#include <Engine/StaticMesh.h>

void MaterialDrawingPlan::Construct(const StaticMeshResources& Resources, 
	const MaterialProxy& MaterialProxy,
	GLUniformBufferRef InLocalToWorldUniform, 
	GraphicsPipeline&& Pipeline)
{
	LocalToWorldUniform = InLocalToWorldUniform;
	IndexCount = Resources.IndexCount;
	IndexBuffer = Resources.IndexBuffer;

	Streams.emplace_back(Resources.PositionBuffer, Pipeline.Vertex->GetAttributeLocation("Position"));
	Streams.emplace_back(Resources.TextureCoordinateBuffer, Pipeline.Vertex->GetAttributeLocation("UV"));
	Streams.emplace_back(Resources.NormalBuffer, Pipeline.Vertex->GetAttributeLocation("Normal"));
	Streams.emplace_back(Resources.TangentBuffer, Pipeline.Vertex->GetAttributeLocation("Tangent"));

	ViewLocation = Pipeline.Vertex->GetUniformLocation("ViewUniform");
	LocalToWorldLocation = Pipeline.Vertex->GetUniformLocation("LocalToWorldUniform");

	CMaterialRef Diffuse = MaterialProxy.Get(EMaterialType::Diffuse);
	Materials.emplace_back(Diffuse->GetMaterial(), Pipeline.Fragment->GetUniformLocation("Diffuse"));

	if (CMaterialRef Normal = MaterialProxy.Get(EMaterialType::Normal); Normal)
	{
		Materials.emplace_back(Normal->GetMaterial(), Pipeline.Fragment->GetUniformLocation("Normal"));
	}
}

void MaterialDrawingPlan::SetUniforms(const View& View, GraphicsPipeline&& Pipeline)
{
	GLSetUniformBuffer(Pipeline.Vertex, ViewLocation, View.Uniform);
	GLSetUniformBuffer(Pipeline.Vertex, LocalToWorldLocation, LocalToWorldUniform);

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
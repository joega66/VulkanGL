#pragma once
#include <Components/CMaterial.h>
#include <Physics/Physics.h>

struct MeshElement
{
	const uint32 IndexCount;
	drm::IndexBufferRef IndexBuffer;

	drm::VertexBufferRef PositionBuffer;
	drm::VertexBufferRef TextureCoordinateBuffer;
	drm::VertexBufferRef NormalBuffer;
	drm::VertexBufferRef TangentBuffer;

	CMaterial Material;
	
	MeshElement(
		uint32 IndexCount
		, drm::IndexBufferRef IndexBuffer
		, drm::VertexBufferRef PositionBuffer
		, drm::VertexBufferRef TextureCoordinateBuffer
		, drm::VertexBufferRef NormalBuffer
		, drm::VertexBufferRef TangentBuffer
		, const CMaterial& Material)
		: IndexCount(IndexCount)
		, IndexBuffer(IndexBuffer)
		, PositionBuffer(PositionBuffer)
		, TextureCoordinateBuffer(TextureCoordinateBuffer)
		, NormalBuffer(NormalBuffer)
		, TangentBuffer(TangentBuffer)
		, Material(Material)
	{
	}
};

struct MeshBatch
{
	std::vector<MeshElement> Elements;
};

class StaticMesh
{
public:
	const std::string Filename;
	const std::string Directory;
	MeshBatch Batch;
	BoundingBox Bounds;

	StaticMesh(const std::string& Filename);

private:
	void LoadStaticMesh();
};

CLASS(StaticMesh);
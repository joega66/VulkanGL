#pragma once
#include <Components/CMaterial.h>
#include <Physics/Physics.h>

struct MeshElement
{
private:
	// Must match StaticMeshVS.glsl
	enum VertexBufferLocation : uint32
	{
		Positions,
		TextureCoordinates,
		Normals,
		Tangents,
		NumLocations
	};
public:
	const uint32 IndexCount;
	drm::IndexBufferRef IndexBuffer;
	std::array<drm::VertexBufferRef, NumLocations> VertexBuffers;

	MeshElement(
		uint32 IndexCount
		, drm::IndexBufferRef IndexBuffer
		, drm::VertexBufferRef PositionBuffer
		, drm::VertexBufferRef TextureCoordinateBuffer
		, drm::VertexBufferRef NormalBuffer
		, drm::VertexBufferRef TangentBuffer)
		: IndexCount(IndexCount)
		, IndexBuffer(IndexBuffer)
	{
		VertexBuffers[Positions] = PositionBuffer;
		VertexBuffers[TextureCoordinates] = TextureCoordinateBuffer;
		VertexBuffers[Normals] = NormalBuffer;
		VertexBuffers[Tangents] = TangentBuffer;
	}

	drm::VertexBufferRef GetPositionBuffer() const { return VertexBuffers[Positions]; }
};

struct MeshBatch
{
	std::vector<MeshElement> Elements;
	std::vector<CMaterial> Materials;
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
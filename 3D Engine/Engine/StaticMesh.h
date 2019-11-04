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
	drm::BufferRef IndexBuffer;
	std::array<drm::BufferRef, NumLocations> VertexBuffers;

	MeshElement(
		uint32 IndexCount
		, drm::BufferRef IndexBuffer
		, drm::BufferRef PositionBuffer
		, drm::BufferRef TextureCoordinateBuffer
		, drm::BufferRef NormalBuffer
		, drm::BufferRef TangentBuffer)
		: IndexCount(IndexCount)
		, IndexBuffer(IndexBuffer)
	{
		VertexBuffers[Positions] = PositionBuffer;
		VertexBuffers[TextureCoordinates] = TextureCoordinateBuffer;
		VertexBuffers[Normals] = NormalBuffer;
		VertexBuffers[Tangents] = TangentBuffer;
	}

	drm::BufferRef GetPositionBuffer() const { return VertexBuffers[Positions]; }
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
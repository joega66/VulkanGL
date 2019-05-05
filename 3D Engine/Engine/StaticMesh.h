#pragma once
#include <Components/CMaterial.h>
#include <Physics/Physics.h>

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;

struct StaticMeshResources
{
	const uint32 IndexCount;
	drm::IndexBufferRef IndexBuffer;

	drm::VertexBufferRef PositionBuffer;
	drm::VertexBufferRef TextureCoordinateBuffer;
	drm::VertexBufferRef NormalBuffer;
	drm::VertexBufferRef TangentBuffer;

	CMaterial Material;
	
	StaticMeshResources(
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

class StaticMesh
{
public:
	const std::string Filename;
	const std::string Directory;
	std::vector<StaticMeshResources> Resources;
	BoundingBox Bounds;

	StaticMesh(const std::string& Filename);
	
private:
	void LoadStaticMesh();
};

CLASS(StaticMesh);
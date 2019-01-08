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
	GLIndexBufferRef IndexBuffer;

	GLVertexBufferRef PositionBuffer;
	GLVertexBufferRef TextureCoordinateBuffer;
	GLVertexBufferRef NormalBuffer;
	GLVertexBufferRef TangentBuffer;

	CMaterial Material;
	
	StaticMeshResources(
		uint32 IndexCount
		, GLIndexBufferRef IndexBuffer
		, GLVertexBufferRef PositionBuffer
		, GLVertexBufferRef TextureCoordinateBuffer
		, GLVertexBufferRef NormalBuffer
		, GLVertexBufferRef TangentBuffer
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
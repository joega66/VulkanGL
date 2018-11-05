#pragma once
#include "Components/CMaterial.h"

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

	MaterialProxyRef Materials;

	StaticMeshResources(
		uint32 IndexCount
		, GLIndexBufferRef IndexBuffer
		, GLVertexBufferRef PositionBuffer
		, GLVertexBufferRef TextureCoordinateBuffer
		, GLVertexBufferRef NormalBuffer
		, GLVertexBufferRef TangentBuffer
		, MaterialProxyRef Materials)
		: IndexCount(IndexCount)
		, IndexBuffer(IndexBuffer)
		, PositionBuffer(PositionBuffer)
		, TextureCoordinateBuffer(TextureCoordinateBuffer)
		, NormalBuffer(NormalBuffer)
		, TangentBuffer(TangentBuffer)
		, Materials(Materials)
	{
	}
};

class StaticMesh
{
public:
	StaticMesh(const std::string& Filename);

	const std::string Filename;
	const std::string Directory;

	std::vector<StaticMeshResources> Resources;
	
private:
	void LoadStaticMesh();

};

CLASS(StaticMesh);
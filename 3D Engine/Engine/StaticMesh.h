#pragma once
#include <Physics/Physics.h>
#include <GPU/GPU.h>
#include "Material.h"
#include <filesystem>

class AssetManager;

class Submesh
{
	// Must match StaticMeshVS.glsl
	enum VertexBufferLocation : uint32
	{
		Positions,
		TextureCoordinates,
		Normals,
		NumLocations
	};
public:
	Submesh(
		uint32 IndexCount
		, EIndexType IndexType
		, gpu::Buffer&& IndexBuffer
		, gpu::Buffer&& PositionBuffer
		, gpu::Buffer&& TextureCoordinateBuffer
		, gpu::Buffer&& NormalBuffer
	) : IndexCount(IndexCount)
		, IndexType(IndexType)
		, IndexBuffer(std::move(IndexBuffer))
	{
		VertexBuffers[Positions] = std::move(PositionBuffer);
		VertexBuffers[TextureCoordinates] = std::move(TextureCoordinateBuffer);
		VertexBuffers[Normals] = std::move(NormalBuffer);
	}

	Submesh(Submesh&& Other)
		: IndexCount(Other.IndexCount)
		, IndexType(Other.IndexType)
		, IndexBuffer(std::move(Other.IndexBuffer))
		, VertexBuffers(std::move(Other.VertexBuffers))
	{}

	Submesh& operator=(Submesh&& Other)
	{
		IndexCount = Other.IndexCount;
		IndexType = Other.IndexType;
		IndexBuffer = std::move(Other.IndexBuffer);
		VertexBuffers = std::move(Other.VertexBuffers);
	}

	inline uint32 GetIndexCount() const { return IndexCount; }
	inline EIndexType GetIndexType() const { return IndexType; }
	inline const gpu::Buffer& GetIndexBuffer() const { return IndexBuffer; }
	inline const std::array<gpu::Buffer, NumLocations>& GetVertexBuffers() const { return VertexBuffers; }
	inline const gpu::Buffer& GetPositionBuffer() const { return VertexBuffers[Positions]; }

private:
	uint32 IndexCount;
	EIndexType IndexType;
	gpu::Buffer IndexBuffer;
	std::array<gpu::Buffer, NumLocations> VertexBuffers;
};

namespace tinygltf { class Model; struct Mesh; struct Primitive; }

class StaticMesh
{
public:
	/** Name of the static mesh asset. */
	const std::string Name;

	/** Path of the static mesh asset. */
	const std::filesystem::path Path;

	/** Submesh data. */
	std::vector<Submesh> Submeshes;
	std::vector<const Material*> Materials;
	std::vector<BoundingBox> SubmeshBounds;
	std::vector<std::string> SubmeshNames;

	/** Load a static mesh from file. */
	StaticMesh(const std::string& AssetName, AssetManager& Assets, gpu::Device& Device, const std::filesystem::path& Path);
	
	/** Initialize a static mesh from a single submesh of a static mesh. */
	StaticMesh(const std::string& AssetName, StaticMesh& StaticMesh, uint32 SubmeshIndex);

	inline const BoundingBox& GetBounds() const { return Bounds; }

private:
	/** Local-space bounds of the mesh. */
	BoundingBox Bounds;

	void GLTFLoad(const std::string& AssetName, AssetManager& Assets, gpu::Device& Device);
	void GLTFLoadGeometry(tinygltf::Model& Model, tinygltf::Mesh& Mesh, tinygltf::Primitive& Primitive, gpu::Device& Device);
	void GLTFLoadMaterial(const std::string& AssetName, AssetManager& Assets, tinygltf::Model& Model, tinygltf::Primitive& Primitive, gpu::Device& Device);
	const gpu::Image* GLTFLoadImage(AssetManager& Assets, gpu::Device& Device, tinygltf::Model& Model, int32 TextureIndex);
};
#pragma once
#include <Physics/Physics.h>
#include <DRM.h>
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
		Tangents,
		NumLocations
	};
public:
	Submesh(
		uint32 IndexCount
		, EIndexType IndexType
		, drm::BufferRef IndexBuffer
		, drm::BufferRef PositionBuffer
		, drm::BufferRef TextureCoordinateBuffer
		, drm::BufferRef NormalBuffer
		, drm::BufferRef TangentBuffer)
		: IndexCount(IndexCount)
		, IndexType(IndexType)
		, IndexBuffer(IndexBuffer)
	{
		VertexBuffers[Positions] = PositionBuffer;
		VertexBuffers[TextureCoordinates] = TextureCoordinateBuffer;
		VertexBuffers[Normals] = NormalBuffer;
		VertexBuffers[Tangents] = TangentBuffer;
	}

	inline uint32 GetIndexCount() const { return IndexCount; }
	inline EIndexType GetIndexType() const { return IndexType; }
	inline drm::BufferRef GetIndexBuffer() const { return IndexBuffer; }
	inline const std::array<drm::BufferRef, NumLocations>& GetVertexBuffers() const { return VertexBuffers; }
	inline drm::BufferRef GetPositionBuffer() const { return VertexBuffers[Positions]; }

private:
	const uint32 IndexCount;
	const EIndexType IndexType;
	drm::BufferRef IndexBuffer;
	std::array<drm::BufferRef, NumLocations> VertexBuffers;
};

namespace tinygltf { class Model; struct Mesh; struct Primitive; }

class StaticMesh
{
public:
	/** Path of the static mesh asset. */
	const std::filesystem::path Path;

	/** Submesh data. */
	std::vector<Submesh> Submeshes;
	std::vector<const Material*> Materials;
	std::vector<BoundingBox> SubmeshBounds;
	std::vector<std::string> SubmeshNames;

	/** Load a static mesh from file. */
	StaticMesh(const std::string& AssetName, AssetManager& Assets, DRMDevice& Device, const std::filesystem::path& Path);
	
	/** Initialize a static mesh from a single submesh of a static mesh. */
	StaticMesh(const StaticMesh& StaticMesh, uint32 SubmeshIndex);

	inline const BoundingBox& GetBounds() const { return Bounds; }

private:
	/** Local-space bounds of the mesh. */
	BoundingBox Bounds;

	void AssimpLoad(AssetManager& Assets, DRMDevice& Device);

	void GLTFLoad(const std::string& AssetName, AssetManager& Assets, DRMDevice& Device);
	void GLTFLoadGeometry(tinygltf::Model& Model, tinygltf::Mesh& Mesh, tinygltf::Primitive& Primitive, DRMDevice& Device);
	void GLTFLoadMaterial(const std::string& AssetName, AssetManager& Assets, tinygltf::Model& Model, tinygltf::Primitive& Primitive, DRMDevice& Device);
	static drm::ImageRef GLTFLoadImage(AssetManager& Assets, DRMDevice& Device, tinygltf::Model& Model, int32 TextureIndex);
};
#pragma once
#include <Components/Material.h>
#include <DRM.h>
#include <Physics/Physics.h>
#include <filesystem>

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

class StaticMesh
{
public:
	/** Filename of the static mesh. */
	const std::filesystem::path Filename;

	/** Directory the file is located in. */
	const std::string Directory;

	/** Submesh data. */
	std::vector<Submesh> Submeshes;
	std::vector<Material> Materials;
	std::vector<BoundingBox> SubmeshBounds;
	std::vector<std::string> SubmeshNames;

	/** Local-space bounds of the mesh. */
	BoundingBox Bounds;

	/** Load a static mesh from file. */
	StaticMesh(class AssetManager& Assets, class DRMDevice& Device, const std::string& Filename);
	
	/** Initialize a static mesh from a single submesh of a static mesh. */
	StaticMesh(const StaticMesh& StaticMesh, uint32 SubmeshIndex);

private:
	void AssimpLoad(class DRMDevice& Device);
	void GLTFLoad(class AssetManager& Assets, class DRMDevice& Device);
};
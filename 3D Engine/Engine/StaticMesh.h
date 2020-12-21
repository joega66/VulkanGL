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
	Submesh(uint32 indexCount
		, EIndexType indexType
		, gpu::Buffer&& indexBuffer
		, gpu::Buffer&& positionBuffer
		, gpu::Buffer&& textureCoordinateBuffer
		, gpu::Buffer&& normalBuffer) 
		: _IndexCount(indexCount)
		, _IndexType(indexType)
		, _IndexBuffer(std::move(indexBuffer))
	{
		_VertexBuffers[Positions] = std::move(positionBuffer);
		_VertexBuffers[TextureCoordinates] = std::move(textureCoordinateBuffer);
		_VertexBuffers[Normals] = std::move(normalBuffer);
	}

	Submesh(Submesh&& other)
		: _IndexCount(other._IndexCount)
		, _IndexType(other._IndexType)
		, _IndexBuffer(std::move(other._IndexBuffer))
		, _VertexBuffers(std::move(other._VertexBuffers))
	{}

	Submesh& operator=(Submesh&& other)
	{
		_IndexCount = other._IndexCount;
		_IndexType = other._IndexType;
		_IndexBuffer = std::move(other._IndexBuffer);
		_VertexBuffers = std::move(other._VertexBuffers);
	}

	inline uint32 GetIndexCount() const { return _IndexCount; }
	inline EIndexType GetIndexType() const { return _IndexType; }
	inline const gpu::Buffer& GetIndexBuffer() const { return _IndexBuffer; }
	inline const std::array<gpu::Buffer, NumLocations>& GetVertexBuffers() const { return _VertexBuffers; }
	inline const gpu::Buffer& GetPositionBuffer() const { return _VertexBuffers[Positions]; }

private:
	uint32 _IndexCount;
	EIndexType _IndexType;
	gpu::Buffer _IndexBuffer;
	std::array<gpu::Buffer, NumLocations> _VertexBuffers;
};

namespace tinygltf { class Model; struct Mesh; struct Primitive; }

class StaticMesh
{
public:
	/** Name of the static mesh asset. */
	const std::string _Name;

	/** Path of the static mesh asset. */
	const std::filesystem::path _Path;

	/** Submesh data. */
	std::vector<Submesh> _Submeshes;
	std::vector<const Material*> _Materials;
	std::vector<BoundingBox> _SubmeshBounds;
	std::vector<std::string> _SubmeshNames;

	/** Load a static mesh from file. */
	StaticMesh(const std::string& assetName, AssetManager& assets, gpu::Device& device, const std::filesystem::path& path);
	
	/** Initialize a static mesh from a single submesh of a static mesh. */
	StaticMesh(const std::string& assetName, StaticMesh& staticMesh, uint32 submeshIndex);

	inline const BoundingBox& GetBounds() const { return _Bounds; }

private:
	/** Local-space bounds of the mesh. */
	BoundingBox _Bounds;

	void GLTFLoad(const std::string& assetName, AssetManager& assets, gpu::Device& device);
	void GLTFLoadGeometry(tinygltf::Model& model, tinygltf::Mesh& mesh, tinygltf::Primitive& primitive, gpu::Device& device);
	void GLTFLoadMaterial(const std::string& assetName, AssetManager& assets, tinygltf::Model& model, tinygltf::Primitive& primitive, gpu::Device& device);
	gpu::Image* GLTFLoadImage(AssetManager& assets, gpu::Device& device, tinygltf::Model& model, int32 textureIndex);
};
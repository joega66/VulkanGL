#include "StaticMesh.h"
#include "AssetManager.h"
#include "../DRM.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using TextureCache = HashTable<std::string, drm::ImageRef>;

static drm::ImageRef LoadMaterials(const std::string& Directory, aiMaterial* AiMaterial, aiTextureType AiType, TextureCache& TextureCache)
{
	for (uint32 TextureIndex = 0; TextureIndex < AiMaterial->GetTextureCount(AiType); TextureIndex++)
	{
		aiString AssimpTexturePath;
		AiMaterial->GetTexture(AiType, TextureIndex, &AssimpTexturePath);
		std::string TexturePath(AssimpTexturePath.C_Str());

		if (!TexturePath.empty())
		{
			if (Contains(TextureCache, TexturePath))
			{
				return TextureCache[TexturePath];
			}

			int32 Width, Height, NumChannels;
			uint8* Pixels = Platform.LoadImage(Directory + "/" + TexturePath, Width, Height, NumChannels);
			drm::ImageRef Material;

			if (Pixels)
			{
				Material = drm::CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled, Pixels);
			}

			Platform.FreeImage(Pixels);

			TextureCache[TexturePath] = Material;

			return Material;
		}
	}

	return nullptr;
}

static Material ProcessMaterials(StaticMesh* StaticMesh, aiMaterial* AiMaterial, TextureCache& TextureCache)
{
	Material Material;

	if (drm::ImageRef Diffuse = LoadMaterials(StaticMesh->Directory, AiMaterial, aiTextureType_DIFFUSE, TextureCache); Diffuse)
	{
		Material.Diffuse = Diffuse;
	}

	if (drm::ImageRef Specular = LoadMaterials(StaticMesh->Directory, AiMaterial, aiTextureType_SPECULAR, TextureCache); Specular)
	{
		Material.Specular = Specular;
	}

	if (drm::ImageRef Opacity = LoadMaterials(StaticMesh->Directory, AiMaterial, aiTextureType_OPACITY, TextureCache); Opacity)
	{
		Material.Opacity = Opacity;
	}

	if (drm::ImageRef Bump = LoadMaterials(StaticMesh->Directory, AiMaterial, aiTextureType_HEIGHT, TextureCache); Bump)
	{
		Material.Bump = Bump;
	}

	return Material;
}

static void ProcessSubmesh(StaticMesh* StaticMesh, aiMesh* AiMesh, const aiScene* AiScene, TextureCache& TextureCache)
{
	check(AiMesh->mTextureCoords[0] > 0, "Static mesh is missing texture coordinates.");

	const uint32 NumVertices = AiMesh->mNumVertices;
	std::vector<glm::vec2> TextureCoordinates(NumVertices);
	std::vector<uint32> Indices;

	BoundingBox Bounds;

	for (uint32 VertexIndex = 0; VertexIndex < NumVertices; VertexIndex++)
	{
		TextureCoordinates[VertexIndex] = glm::vec2(AiMesh->mTextureCoords[0][VertexIndex].x, AiMesh->mTextureCoords[0][VertexIndex].y);

		const aiVector3D& AiPosition = AiMesh->mVertices[VertexIndex];
		const glm::vec3 Position = glm::vec3(AiPosition.x, AiPosition.y, AiPosition.z);

		Bounds.TestPoint(Position);
	}

	for (uint32 FaceIndex = 0; FaceIndex < AiMesh->mNumFaces; FaceIndex++)
	{
		const aiFace& Face = AiMesh->mFaces[FaceIndex];

		for (uint32 j = 0; j < Face.mNumIndices; j++)
		{
			Indices.push_back(Face.mIndices[j]);
		}
	}

	Material Materials = ProcessMaterials(StaticMesh, AiScene->mMaterials[AiMesh->mMaterialIndex], TextureCache);

	drm::BufferRef IndexBuffer = drm::CreateBuffer(EBufferUsage::Index, Indices.size() * sizeof(uint32), Indices.data());
	drm::BufferRef PositionBuffer = drm::CreateBuffer(EBufferUsage::Vertex, AiMesh->mNumVertices * sizeof(glm::vec3), AiMesh->mVertices);
	drm::BufferRef TextureCoordinateBuffer = drm::CreateBuffer(EBufferUsage::Vertex, TextureCoordinates.size() * sizeof(glm::vec2), TextureCoordinates.data());
	drm::BufferRef NormalBuffer = drm::CreateBuffer(EBufferUsage::Vertex, AiMesh->mNumVertices * sizeof(glm::vec3), AiMesh->mNormals);
	drm::BufferRef TangentBuffer = drm::CreateBuffer(EBufferUsage::Vertex, AiMesh->mNumVertices * sizeof(glm::vec3), AiMesh->mTangents);

	StaticMesh->Submeshes.emplace_back(Submesh(
		Indices.size()
		, IndexBuffer
		, PositionBuffer
		, TextureCoordinateBuffer
		, NormalBuffer
		, TangentBuffer));
	StaticMesh->Materials.push_back(Materials);
	StaticMesh->SubmeshBounds.push_back(Bounds);
	StaticMesh->SubmeshNames.push_back(std::string(AiMesh->mName.C_Str()));
}

void ProcessNode(StaticMesh* StaticMesh, const aiNode* AiNode, const aiScene* AiScene, TextureCache& TextureCache)
{
	for (uint32 MeshIndex = 0; MeshIndex < AiNode->mNumMeshes; MeshIndex++)
	{
		aiMesh* AiMesh = AiScene->mMeshes[AiNode->mMeshes[MeshIndex]];
		ProcessSubmesh(StaticMesh, AiMesh, AiScene, TextureCache);
	}

	for (uint32 i = 0; i < AiNode->mNumChildren; i++)
	{
		ProcessNode(StaticMesh, AiNode->mChildren[i], AiScene, TextureCache);
	}
}

StaticMesh::StaticMesh(const std::string& Filename)
	: Filename(Filename), Directory(Filename.substr(0, Filename.find_last_of("/")))
{
	uint32 AssimpLoadFlags = 0;

	AssimpLoadFlags |= aiProcess_Triangulate
		| aiProcess_JoinIdenticalVertices
		| aiProcess_OptimizeMeshes
		| aiProcess_OptimizeGraph
		| aiProcess_FlipUVs
		| aiProcess_CalcTangentSpace;

	Assimp::Importer Importer;
	const aiScene* AiScene = Importer.ReadFile(Filename, AssimpLoadFlags);

	if (!AiScene || AiScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !AiScene->mRootNode)
	{
		std::string AssimpError = Importer.GetErrorString();
		fail("Assimp error: %s", AssimpError.c_str());
		return;
	}

	TextureCache TextureCache;
	ProcessNode(this, AiScene->mRootNode, AiScene, TextureCache);

	std::for_each(SubmeshBounds.begin(), SubmeshBounds.end(), [&] (const BoundingBox& SubmeshBounds)
	{
		Bounds.TestPoint(SubmeshBounds.GetMin());
		Bounds.TestPoint(SubmeshBounds.GetMax());
	});
}

StaticMesh::StaticMesh(const StaticMesh& StaticMesh, uint32 SubmeshIndex)
	: Filename(StaticMesh.Filename)
	, Directory(StaticMesh.Directory)
	, Submeshes{ StaticMesh.Submeshes[SubmeshIndex] }
	, Materials{ StaticMesh.Materials[SubmeshIndex] }
	, SubmeshBounds{ StaticMesh.SubmeshBounds[SubmeshIndex] }
	, SubmeshNames{ StaticMesh.SubmeshNames[SubmeshIndex] }
	, Bounds{ StaticMesh.SubmeshBounds[SubmeshIndex] }
{
}

#include "StaticMesh.h"
#include "AssetManager.h"
#include "../DRM.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using TextureCache = HashTable<std::string, drm::ImageRef>;

drm::ImageRef LoadMaterials(const std::string& Directory, aiMaterial* AiMaterial, aiTextureType AiType, TextureCache& TextureCache)
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
				Material = drm::CreateImage(Width, Height, EImageFormat::R8G8B8A8_UNORM, EResourceUsage::ShaderResource, Pixels);
			}

			Platform.FreeImage(Pixels);

			TextureCache[TexturePath] = Material;

			return Material;
		}
	}

	return nullptr;
}

CMaterial ProcessMaterials(StaticMesh* StaticMesh, aiMaterial* AiMaterial, TextureCache& TextureCache)
{
	CMaterial Material;

	if (drm::ImageRef DiffuseMap = LoadMaterials(StaticMesh->Directory, AiMaterial, aiTextureType_DIFFUSE, TextureCache); DiffuseMap)
	{
		Material.Diffuse = DiffuseMap;
	}
	else
	{
		Material.Diffuse = GAssetManager.GetImage("Engine-Diffuse-Default");
	}

	if (drm::ImageRef NormalMap = LoadMaterials(StaticMesh->Directory, AiMaterial, aiTextureType_NORMALS, TextureCache); NormalMap)
	{
		Material.Normal = NormalMap;
	}

	return Material;
}

void ProcessMesh(StaticMesh* StaticMesh, aiMesh* AiMesh, const aiScene* AiScene, TextureCache& TextureCache)
{
	check(AiMesh->mTextureCoords[0] > 0, "Static mesh is missing texture coordinates.");

	const uint32 NumVertices = AiMesh->mNumVertices;

	std::vector<glm::vec3> Positions(NumVertices);
	std::vector<glm::vec2> TextureCoordinates(NumVertices);
	std::vector<glm::vec3> Normals(NumVertices);
	std::vector<glm::vec3> Tangents(NumVertices);
	std::vector<uint32> Indices;

	glm::vec3& Min = StaticMesh->Bounds.Min;
	glm::vec3& Max = StaticMesh->Bounds.Max;

	for (uint32 i = 0; i < NumVertices; i++)
	{
		Positions[i] = glm::vec3(AiMesh->mVertices[i].x, AiMesh->mVertices[i].y, AiMesh->mVertices[i].z);
		TextureCoordinates[i] = glm::vec2(AiMesh->mTextureCoords[0][i].x, AiMesh->mTextureCoords[0][i].y);
		Normals[i] = glm::vec3(AiMesh->mNormals[i].x, AiMesh->mNormals[i].y, AiMesh->mNormals[i].z);
		Tangents[i] = glm::vec3(AiMesh->mTangents[i].x, AiMesh->mTangents[i].y, AiMesh->mTangents[i].z);

		const glm::vec3& Position = Positions[i];
		if (Position.x > Max.x)
			Max.x = Position.x;
		if (Position.y > Max.y)
			Max.y = Position.y;
		if (Position.z > Max.z)
			Max.z = Position.z;
		if (Position.x < Min.x)
			Min.x = Position.x;
		if (Position.y < Min.y)
			Min.y = Position.y;
		if (Position.z < Min.z)
			Min.z = Position.z;
	}

	for (uint32 i = 0; i < AiMesh->mNumFaces; i++)
	{
		const aiFace& Face = AiMesh->mFaces[i];

		for (uint32 j = 0; j < Face.mNumIndices; j++)
		{
			Indices.push_back(Face.mIndices[j]);
		}
	}

	CMaterial Materials = ProcessMaterials(StaticMesh, AiScene->mMaterials[AiMesh->mMaterialIndex], TextureCache);

	drm::IndexBufferRef IndexBuffer = drm::CreateIndexBuffer(EImageFormat::R32_UINT, Indices.size(), EResourceUsage::None, Indices.data());
	drm::VertexBufferRef PositionBuffer = drm::CreateVertexBuffer(EImageFormat::R32G32B32_SFLOAT, Positions.size(), EResourceUsage::None, Positions.data());
	drm::VertexBufferRef TextureCoordinateBuffer = drm::CreateVertexBuffer(EImageFormat::R32G32_SFLOAT, TextureCoordinates.size(), EResourceUsage::None, TextureCoordinates.data());
	drm::VertexBufferRef NormalBuffer = drm::CreateVertexBuffer(EImageFormat::R32G32B32_SFLOAT, Normals.size(), EResourceUsage::None, Normals.data());
	drm::VertexBufferRef TangentBuffer = drm::CreateVertexBuffer(EImageFormat::R32G32B32_SFLOAT, Tangents.size(), EResourceUsage::None, Tangents.data());

	StaticMesh->Batch.Elements.emplace_back(MeshElement(
		Indices.size()
		, IndexBuffer
		, PositionBuffer
		, TextureCoordinateBuffer
		, NormalBuffer
		, TangentBuffer
		, Materials));
}

void ProcessNode(StaticMesh* StaticMesh, const aiNode* AiNode, const aiScene* AiScene, TextureCache& TextureCache)
{
	for (uint32 i = 0; i < AiNode->mNumMeshes; i++)
	{
		aiMesh* AiMesh = AiScene->mMeshes[AiNode->mMeshes[i]];
		ProcessMesh(StaticMesh, AiMesh, AiScene, TextureCache);
	}

	for (uint32 i = 0; i < AiNode->mNumChildren; i++)
	{
		ProcessNode(StaticMesh, AiNode->mChildren[i], AiScene, TextureCache);
	}
}

StaticMesh::StaticMesh(const std::string& Filename)
	: Filename(Filename), Directory(Filename.substr(0, Filename.find_last_of("/")))
{
	LoadStaticMesh();
}

void StaticMesh::LoadStaticMesh()
{
	uint32 AssimpLoadFlags = 0;

	AssimpLoadFlags |= aiProcess_Triangulate | aiProcess_JoinIdenticalVertices
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
}
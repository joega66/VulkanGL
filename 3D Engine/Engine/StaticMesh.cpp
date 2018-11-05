#include "StaticMesh.h"
#include "ResourceManager.h"
#include "../GL.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using TextureCache = Map<std::string, GLImageRef>;

GLImageRef LoadMaterials(const std::string& Directory, aiMaterial* AiMaterial, aiTextureType AiType, TextureCache& TextureCache)
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
			uint8* Pixels = GPlatform->LoadImage(Directory + "/" + TexturePath, Width, Height, NumChannels);
			GLImageRef Material;

			if (Pixels)
			{
				Material = GLCreateImage(Width, Height, IF_R8G8B8A8_UNORM, RU_ShaderResource, Pixels);
			}

			GPlatform->FreeImage(Pixels);

			TextureCache[TexturePath] = Material;

			return Material;
		}
	}

	return nullptr;
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

	for (uint32 i = 0; i < NumVertices; i++)
	{
		Positions[i] = glm::vec3(AiMesh->mVertices[i].x, AiMesh->mVertices[i].y, AiMesh->mVertices[i].z);
		TextureCoordinates[i] = glm::vec2(AiMesh->mTextureCoords[0][i].x, AiMesh->mTextureCoords[0][i].y);
		Normals[i] = glm::vec3(AiMesh->mNormals[i].x, AiMesh->mNormals[i].y, AiMesh->mNormals[i].z);
		Tangents[i] = glm::vec3(AiMesh->mTangents[i].x, AiMesh->mTangents[i].y, AiMesh->mTangents[i].z);
	}

	for (uint32 i = 0; i < AiMesh->mNumFaces; i++)
	{
		aiFace Face = AiMesh->mFaces[i];

		for (uint32 j = 0; j < Face.mNumIndices; j++)
		{
			Indices.push_back(Face.mIndices[j]);
		}
	}

	aiMaterial* Material = AiScene->mMaterials[AiMesh->mMaterialIndex];
	MaterialProxyRef Materials = MakeRef<MaterialProxy>();

	GLImageRef DiffuseMap = LoadMaterials(StaticMesh->Directory, Material, aiTextureType_DIFFUSE, TextureCache);

	if (DiffuseMap)
	{
		Materials->Add(MakeRef<CMaterial>(DiffuseMap, EMaterialType::Diffuse));
	}
	else
	{
		Materials->Add(MakeRef<CMaterial>(GAssetManager->GetImage("Engine-Diffuse-Default"), EMaterialType::Diffuse));
	}

	GLImageRef NormalMap = LoadMaterials(StaticMesh->Directory, Material, aiTextureType_NORMALS, TextureCache);

	if (NormalMap)
	{
		Materials->Add(MakeRef<CMaterial>(NormalMap, EMaterialType::Normal));
	}

	GLIndexBufferRef IndexBuffer = GLCreateIndexBuffer(IF_R32_UINT, Indices.size(), RU_None, Indices.data());
	GLVertexBufferRef PositionBuffer = GLCreateVertexBuffer(IF_R32G32B32_SFLOAT, Positions.size(), RU_None, Positions.data());
	GLVertexBufferRef TextureCoordinateBuffer = GLCreateVertexBuffer(IF_R32G32_SFLOAT, TextureCoordinates.size(), RU_None, TextureCoordinates.data());
	GLVertexBufferRef NormalBuffer = GLCreateVertexBuffer(IF_R32G32B32_SFLOAT, Normals.size(), RU_None, Normals.data());
	GLVertexBufferRef TangentBuffer = GLCreateVertexBuffer(IF_R32G32B32_SFLOAT, Tangents.size(), RU_None, Tangents.data());

	StaticMesh->Resources.emplace_back(StaticMeshResources(
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
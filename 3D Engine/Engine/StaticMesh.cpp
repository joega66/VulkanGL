#include "StaticMesh.h"
#include "AssetManager.h"
#include "../DRM.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using TextureCache = HashTable<std::string, drm::ImageRef>;

static drm::ImageRef LoadMaterials(DRMDevice& Device, const std::string& Directory, aiMaterial* AiMaterial, aiTextureType AiType, TextureCache& TextureCache)
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
			uint8* Pixels = Platform::LoadImage(Directory + "/" + TexturePath, Width, Height, NumChannels);
			drm::ImageRef Material;

			if (Pixels)
			{
				Material = Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);

				drm::BufferRef StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, Material->GetSize(), Pixels);

				drm::CommandListRef CmdList = Device.CreateCommandList();

				ImageMemoryBarrier Barrier(
					Material,
					EAccess::None,
					EAccess::TransferWrite,
					EImageLayout::Undefined,
					EImageLayout::TransferDstOptimal
				);

				CmdList->PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

				CmdList->CopyBufferToImage(StagingBuffer, 0, Material, EImageLayout::TransferDstOptimal);

				Barrier.SrcAccessMask = EAccess::TransferWrite;
				Barrier.DstAccessMask = EAccess::ShaderRead;
				Barrier.OldLayout = EImageLayout::TransferDstOptimal;
				Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;

				CmdList->PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &Barrier);

				Device.SubmitCommands(CmdList);
			}

			Platform::FreeImage(Pixels);

			TextureCache[TexturePath] = Material;

			return Material;
		}
	}

	return nullptr;
}

static Material ProcessMaterials(DRMDevice& Device, StaticMesh* StaticMesh, aiMaterial* AiMaterial, TextureCache& TextureCache)
{
	Material Material;

	if (drm::ImageRef Diffuse = LoadMaterials(Device, StaticMesh->Directory, AiMaterial, aiTextureType_DIFFUSE, TextureCache); Diffuse)
	{
		Material.MaterialSet.Diffuse = Diffuse;
	}

	if (drm::ImageRef Specular = LoadMaterials(Device, StaticMesh->Directory, AiMaterial, aiTextureType_SPECULAR, TextureCache); Specular)
	{
		Material.MaterialSet.Specular = Specular;
	}

	if (drm::ImageRef Opacity = LoadMaterials(Device, StaticMesh->Directory, AiMaterial, aiTextureType_OPACITY, TextureCache); Opacity)
	{
		Material.MaterialSet.Opacity = Opacity;
	}

	if (drm::ImageRef Bump = LoadMaterials(Device, StaticMesh->Directory, AiMaterial, aiTextureType_HEIGHT, TextureCache); Bump)
	{
		Material.MaterialSet.Bump = Bump;
	}

	return Material;
}

static void ProcessSubmesh(DRMDevice& Device, StaticMesh* StaticMesh, aiMesh* AiMesh, const aiScene* AiScene, TextureCache& TextureCache)
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

	Material Materials = ProcessMaterials(Device, StaticMesh, AiScene->mMaterials[AiMesh->mMaterialIndex], TextureCache);

	drm::BufferRef IndexBuffer = Device.CreateBuffer(EBufferUsage::Index, Indices.size() * sizeof(uint32), Indices.data());
	drm::BufferRef PositionBuffer = Device.CreateBuffer(EBufferUsage::Vertex, AiMesh->mNumVertices * sizeof(glm::vec3), AiMesh->mVertices);
	drm::BufferRef TextureCoordinateBuffer = Device.CreateBuffer(EBufferUsage::Vertex, TextureCoordinates.size() * sizeof(glm::vec2), TextureCoordinates.data());
	drm::BufferRef NormalBuffer = Device.CreateBuffer(EBufferUsage::Vertex, AiMesh->mNumVertices * sizeof(glm::vec3), AiMesh->mNormals);
	drm::BufferRef TangentBuffer = Device.CreateBuffer(EBufferUsage::Vertex, AiMesh->mNumVertices * sizeof(glm::vec3), AiMesh->mTangents);

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

void ProcessNode(DRMDevice& Device, StaticMesh* StaticMesh, const aiNode* AiNode, const aiScene* AiScene, TextureCache& TextureCache)
{
	for (uint32 MeshIndex = 0; MeshIndex < AiNode->mNumMeshes; MeshIndex++)
	{
		aiMesh* AiMesh = AiScene->mMeshes[AiNode->mMeshes[MeshIndex]];
		ProcessSubmesh(Device, StaticMesh, AiMesh, AiScene, TextureCache);
	}

	for (uint32 i = 0; i < AiNode->mNumChildren; i++)
	{
		ProcessNode(Device, StaticMesh, AiNode->mChildren[i], AiScene, TextureCache);
	}
}

StaticMesh::StaticMesh(DRMDevice& Device, const std::string& Filename)
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
	ProcessNode(Device, this, AiScene->mRootNode, AiScene, TextureCache);

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

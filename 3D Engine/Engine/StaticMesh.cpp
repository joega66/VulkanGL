#include "StaticMesh.h"
#include "AssetManager.h"
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

				drm::UploadImageData(Device, Pixels, Material);
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
	MaterialDescriptors Descriptors;

	if (drm::ImageRef BaseColor = LoadMaterials(Device, StaticMesh->Directory, AiMaterial, aiTextureType_DIFFUSE, TextureCache); BaseColor)
	{
		Descriptors.BaseColor = BaseColor;
	}

	Material Material(Device, Descriptors, EMaterialMode::Opaque, 0.0f, 0.0f); // Only support Opaque for OBJ because the format is annoying

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

	drm::BufferRef IndexBuffer = Device.CreateBuffer(EBufferUsage::Index, Indices.size() * sizeof(uint32));
	drm::BufferRef PositionBuffer = Device.CreateBuffer(EBufferUsage::Vertex, AiMesh->mNumVertices * sizeof(glm::vec3));
	drm::BufferRef TextureCoordinateBuffer = Device.CreateBuffer(EBufferUsage::Vertex, TextureCoordinates.size() * sizeof(glm::vec2));
	drm::BufferRef NormalBuffer = Device.CreateBuffer(EBufferUsage::Vertex, AiMesh->mNumVertices * sizeof(glm::vec3));
	drm::BufferRef TangentBuffer = Device.CreateBuffer(EBufferUsage::Vertex, AiMesh->mNumVertices * sizeof(glm::vec3));

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

	uint32 SrcOffset = 0;

	drm::BufferRef StagingBuffer = Device.CreateBuffer(
		EBufferUsage::Transfer, IndexBuffer->GetSize() + PositionBuffer->GetSize() + TextureCoordinateBuffer->GetSize() + NormalBuffer->GetSize() + TangentBuffer->GetSize()
	);

	uint8* Memmapped = static_cast<uint8*>(Device.LockBuffer(StagingBuffer));

	Platform::Memcpy(Memmapped, Indices.data(), Indices.size() * sizeof(uint32));
	CmdList.CopyBuffer(StagingBuffer, IndexBuffer, SrcOffset, 0, IndexBuffer->GetSize());
	Memmapped += IndexBuffer->GetSize();
	SrcOffset += IndexBuffer->GetSize();

	Platform::Memcpy(Memmapped, AiMesh->mVertices, PositionBuffer->GetSize());
	CmdList.CopyBuffer(StagingBuffer, PositionBuffer, SrcOffset, 0, PositionBuffer->GetSize());
	Memmapped += PositionBuffer->GetSize();
	SrcOffset += PositionBuffer->GetSize();

	Platform::Memcpy(Memmapped, TextureCoordinates.data(), TextureCoordinateBuffer->GetSize());
	CmdList.CopyBuffer(StagingBuffer, TextureCoordinateBuffer, SrcOffset, 0, TextureCoordinateBuffer->GetSize());
	Memmapped += TextureCoordinateBuffer->GetSize();
	SrcOffset += TextureCoordinateBuffer->GetSize();

	Platform::Memcpy(Memmapped, AiMesh->mNormals, NormalBuffer->GetSize());
	CmdList.CopyBuffer(StagingBuffer, NormalBuffer, SrcOffset, 0, NormalBuffer->GetSize());
	Memmapped += NormalBuffer->GetSize();
	SrcOffset += NormalBuffer->GetSize();

	Platform::Memcpy(Memmapped, AiMesh->mTangents, TangentBuffer->GetSize());
	CmdList.CopyBuffer(StagingBuffer, TangentBuffer, SrcOffset, 0, TangentBuffer->GetSize());
	Memmapped += TangentBuffer->GetSize();
	SrcOffset += TangentBuffer->GetSize();

	Device.UnlockBuffer(StagingBuffer);

	Device.SubmitCommands(CmdList);

	StaticMesh->Submeshes.emplace_back(Submesh(
		Indices.size()
		, EIndexType::UINT32
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

StaticMesh::StaticMesh(AssetManager& Assets, DRMDevice& Device, const std::string& FilenameStr)
	: Filename(FilenameStr), Directory(FilenameStr.substr(0, FilenameStr.find_last_of("/")))
{
	if (Filename.extension() == ".obj")
	{
		AssimpLoad(Device);
	}
	else if (Filename.extension() == ".bin" || Filename.extension() == ".gltf")
	{
		GLTFLoad(Assets, Device);
	}

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

void StaticMesh::AssimpLoad(DRMDevice& Device)
{
	uint32 AssimpLoadFlags = 0;

	AssimpLoadFlags |= aiProcess_Triangulate
		| aiProcess_JoinIdenticalVertices
		| aiProcess_OptimizeMeshes
		| aiProcess_OptimizeGraph
		| aiProcess_FlipUVs
		| aiProcess_CalcTangentSpace;

	Assimp::Importer Importer;
	const aiScene* AiScene = Importer.ReadFile(Filename.generic_string(), AssimpLoadFlags);

	if (!AiScene || AiScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !AiScene->mRootNode)
	{
		std::string AssimpError = Importer.GetErrorString();
		fail("Assimp error: %s", AssimpError.c_str());
		return;
	}

	TextureCache TextureCache;
	ProcessNode(Device, this, AiScene->mRootNode, AiScene, TextureCache);
}

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <tiny_gltf.h>

tinygltf::TinyGLTF Loader;

static EFormat GetFormat(int32 Bits, int32 Components, int32 PixelType)
{
	if (PixelType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE && Bits == 8 && Components == 4)
	{
		return EFormat::R8G8B8A8_UNORM;
	}
	else
	{
		signal_unimplemented();
	}
}

static drm::ImageRef LoadImageFromGLTF(AssetManager& Assets, DRMDevice& Device, tinygltf::Model& Model, int32 TextureIndex)
{
	if (TextureIndex == -1)
	{
		return Material::Dummy;
	}
	else
	{
		auto& Texture = Model.textures[TextureIndex];
		auto& Image = Model.images[Texture.source];

		if (drm::ImageRef LoadedImage = Assets.GetImage(Image.uri); LoadedImage != nullptr)
		{
			return LoadedImage;
		}
		else
		{
			drm::ImageRef NewImage = Device.CreateImage(Image.width, Image.height, 1, GetFormat(Image.bits, Image.component, Image.pixel_type), EImageUsage::Sampled | EImageUsage::TransferDst);
			drm::UploadImageData(Device, Image.image.data(), NewImage);
#undef LoadImage
			Assets.LoadImage(Image.uri, NewImage);
			return NewImage;
		}
	}
}

void StaticMesh::GLTFLoad(AssetManager& Assets, DRMDevice& Device)
{
	tinygltf::Model Model;
	std::string Err;
	std::string Warn;
	
	Loader.LoadASCIIFromFile(&Model, &Err, &Warn, Filename.generic_string());

	if (!Err.empty())
	{
		LOG("TinyGLTF error: %s", Err.c_str());
	}
	if (!Warn.empty())
	{
		LOG("TinyGLTF warning: %s", Warn.c_str());
	}

	for (auto& Mesh : Model.meshes)
	{
		for (auto& Primitive : Mesh.primitives)
		{
			auto& IndexAccessor = Model.accessors[Primitive.indices];
			auto& PositionAccessor = Model.accessors[Primitive.attributes["POSITION"]];
			auto& NormalAccessor = Model.accessors[Primitive.attributes["NORMAL"]];
			auto& UvAccessor = Model.accessors[Primitive.attributes["TEXCOORD_0"]];

			auto& IndexView = Model.bufferViews[IndexAccessor.bufferView];
			auto& PositionView = Model.bufferViews[PositionAccessor.bufferView];
			auto& NormalView = Model.bufferViews[NormalAccessor.bufferView];
			auto& UvView = Model.bufferViews[UvAccessor.bufferView];

			check(IndexView.byteStride == 0 && PositionView.byteStride == 0 && NormalView.byteStride == 0 && UvView.byteStride == 0,
				"Need to add support for nonzero strides...");

			auto& IndexData = Model.buffers[IndexView.buffer];
			auto& PositionData = Model.buffers[PositionView.buffer];
			auto& NormalData = Model.buffers[NormalView.buffer];
			auto& UvData = Model.buffers[UvView.buffer];

			drm::BufferRef IndexBuffer = Device.CreateBuffer(EBufferUsage::Index, IndexView.byteLength);
			drm::BufferRef PositionBuffer = Device.CreateBuffer(EBufferUsage::Vertex, PositionView.byteLength);
			drm::BufferRef TextureCoordinateBuffer = Device.CreateBuffer(EBufferUsage::Vertex, UvView.byteLength);
			drm::BufferRef NormalBuffer = Device.CreateBuffer(EBufferUsage::Vertex, NormalView.byteLength);

			{
				drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

				uint32 SrcOffset = 0;

				// Create one big staging buffer for the upload, because why not.
				drm::BufferRef StagingBuffer = Device.CreateBuffer(
					EBufferUsage::Transfer,
					IndexView.byteLength + PositionView.byteLength + UvView.byteLength + NormalView.byteLength
				);

				uint8* Memmapped = static_cast<uint8*>(Device.LockBuffer(StagingBuffer));

				Platform::Memcpy(Memmapped, IndexData.data.data() + IndexView.byteOffset, IndexView.byteLength);
				CmdList.CopyBuffer(StagingBuffer, IndexBuffer, SrcOffset, 0, IndexView.byteLength);
				Memmapped += IndexView.byteLength;
				SrcOffset += IndexView.byteLength;

				Platform::Memcpy(Memmapped, PositionData.data.data() + PositionView.byteOffset, PositionView.byteLength);
				CmdList.CopyBuffer(StagingBuffer, PositionBuffer, SrcOffset, 0, PositionView.byteLength);
				Memmapped += PositionView.byteLength;
				SrcOffset += PositionView.byteLength;

				Platform::Memcpy(Memmapped, UvData.data.data() + UvView.byteOffset, UvView.byteLength);
				CmdList.CopyBuffer(StagingBuffer, TextureCoordinateBuffer, SrcOffset, 0, UvView.byteLength);
				Memmapped += UvView.byteLength;
				SrcOffset += UvView.byteLength;

				Platform::Memcpy(Memmapped, NormalData.data.data() + NormalView.byteOffset, NormalView.byteLength);
				CmdList.CopyBuffer(StagingBuffer, NormalBuffer, SrcOffset, 0, NormalView.byteLength);
				Memmapped += NormalView.byteLength;
				SrcOffset += NormalView.byteLength;

				Device.UnlockBuffer(StagingBuffer);

				Device.SubmitCommands(CmdList);
			}
			
			Submeshes.emplace_back(Submesh(
				IndexAccessor.count
				, tinygltf::GetComponentSizeInBytes(IndexAccessor.componentType) == 2 ? EIndexType::UINT16 : EIndexType::UINT32
				, IndexBuffer
				, PositionBuffer
				, TextureCoordinateBuffer
				, NormalBuffer
				, NormalBuffer) // @todo Generate tangents
			);

			auto& GLTFMaterial = Model.materials[Primitive.material];

			const EMaterialMode MaterialMode = [](const std::string& AlphaMode)
			{
				if (AlphaMode == "MASK")
				{
					return EMaterialMode::Masked;
				}
				else
				{
					return EMaterialMode::Opaque;
				}
			}(GLTFMaterial.alphaMode);

			MaterialDescriptors Descriptors;
			Descriptors.BaseColor = LoadImageFromGLTF(Assets, Device, Model, GLTFMaterial.pbrMetallicRoughness.baseColorTexture.index);
			Descriptors.MetallicRoughness = LoadImageFromGLTF(Assets, Device, Model, GLTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);

			Material Material(
				Device,
				Descriptors,
				MaterialMode,
				static_cast<float>(GLTFMaterial.pbrMetallicRoughness.roughnessFactor), 
				static_cast<float>(GLTFMaterial.pbrMetallicRoughness.metallicFactor)
			);

			Materials.push_back(Material);

			const glm::vec3 Min(PositionAccessor.minValues[0], PositionAccessor.minValues[1], PositionAccessor.minValues[2]);
			const glm::vec3 Max(PositionAccessor.maxValues[0], PositionAccessor.maxValues[1], PositionAccessor.maxValues[2]);
			
			SubmeshBounds.push_back(BoundingBox(Min, Max));
			
			SubmeshNames.push_back(Mesh.name);
		}
	}
}
#include "StaticMesh.h"
#include "AssetManager.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <tiny_gltf.h>

StaticMesh::StaticMesh(const std::string& AssetName, AssetManager& Assets, DRMDevice& Device, const std::filesystem::path& Path)
	: Path(Path)
{
	if (Path.extension() == ".gltf" || Path.extension() == ".glb")
	{
		GLTFLoad(AssetName, Assets, Device);
	}
	else
	{
		signal_unimplemented();
	}

	std::for_each(SubmeshBounds.begin(), SubmeshBounds.end(), [&] (const BoundingBox& SubmeshBounds)
	{
		Bounds.TestPoint(SubmeshBounds.GetMin());
		Bounds.TestPoint(SubmeshBounds.GetMax());
	});
}

StaticMesh::StaticMesh(StaticMesh& StaticMesh, uint32 SubmeshIndex)
	: Path(StaticMesh.Path)
	, Materials{ StaticMesh.Materials[SubmeshIndex] }
	, SubmeshBounds{ StaticMesh.SubmeshBounds[SubmeshIndex] }
	, SubmeshNames{ StaticMesh.SubmeshNames[SubmeshIndex] }
	, Bounds{ StaticMesh.SubmeshBounds[SubmeshIndex] }
{
	Submeshes.emplace_back(std::move(StaticMesh.Submeshes[SubmeshIndex]));
}

tinygltf::TinyGLTF Loader;

void StaticMesh::GLTFLoad(const std::string& AssetName, AssetManager& Assets, DRMDevice& Device)
{
	tinygltf::Model Model;
	std::string Err;
	std::string Warn;

	if (Path.extension() == ".gltf")
	{
		Loader.LoadASCIIFromFile(&Model, &Err, &Warn, Path.generic_string());
	}
	else
	{
		Loader.LoadBinaryFromFile(&Model, &Err, &Warn, Path.generic_string());
	}

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
			GLTFLoadGeometry(Model, Mesh, Primitive, Device);
			GLTFLoadMaterial(AssetName, Assets, Model, Primitive, Device);
			SubmeshNames.push_back(Mesh.name);
		}
	}
}

void StaticMesh::GLTFLoadGeometry(tinygltf::Model& Model, tinygltf::Mesh& Mesh, tinygltf::Primitive& Primitive, DRMDevice& Device)
{
	auto& IndexAccessor = Model.accessors[Primitive.indices];
	auto& PositionAccessor = Model.accessors[Primitive.attributes["POSITION"]];
	auto& NormalAccessor = Model.accessors[Primitive.attributes["NORMAL"]];
	auto& UvAccessor = Model.accessors[Primitive.attributes["TEXCOORD_0"]];

	auto& IndexView = Model.bufferViews[IndexAccessor.bufferView];
	auto& PositionView = Model.bufferViews[PositionAccessor.bufferView];
	auto& NormalView = Model.bufferViews[NormalAccessor.bufferView];
	auto& UvView = Model.bufferViews[UvAccessor.bufferView];

	auto& IndexData = Model.buffers[IndexView.buffer];
	auto& PositionData = Model.buffers[PositionView.buffer];
	auto& NormalData = Model.buffers[NormalView.buffer];
	auto& UvData = Model.buffers[UvView.buffer];

	drm::Buffer IndexBuffer = Device.CreateBuffer(EBufferUsage::Index, IndexView.byteLength);
	drm::Buffer PositionBuffer = Device.CreateBuffer(EBufferUsage::Vertex, PositionView.byteLength);
	drm::Buffer TextureCoordinateBuffer = Device.CreateBuffer(EBufferUsage::Vertex, UvView.byteLength);
	drm::Buffer NormalBuffer = Device.CreateBuffer(EBufferUsage::Vertex, NormalView.byteLength);
	drm::Buffer TangentBuffer = Device.CreateBuffer(EBufferUsage::Vertex, NormalView.byteLength);

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

	uint64 SrcOffset = 0;

	// Create one big staging buffer for the upload, because why not.
	drm::Buffer StagingBuffer = Device.CreateBuffer(
		EBufferUsage::Transfer,
		IndexView.byteLength + PositionView.byteLength + UvView.byteLength + NormalView.byteLength
	);

	uint8* Memmapped = static_cast<uint8*>(StagingBuffer.GetData());

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
	CmdList.CopyBuffer(StagingBuffer, TangentBuffer, SrcOffset, 0, NormalView.byteLength); // @todo Generate tangents
	Memmapped += NormalView.byteLength;
	SrcOffset += NormalView.byteLength;

	Device.SubmitCommands(CmdList);

	Submeshes.emplace_back(Submesh(
		static_cast<uint32>(IndexAccessor.count)
		, tinygltf::GetComponentSizeInBytes(IndexAccessor.componentType) == 2 ? EIndexType::UINT16 : EIndexType::UINT32
		, std::move(IndexBuffer)
		, std::move(PositionBuffer)
		, std::move(TextureCoordinateBuffer)
		, std::move(NormalBuffer)
		, std::move(TangentBuffer))
	);

	const glm::vec3 Min(PositionAccessor.minValues[0], PositionAccessor.minValues[1], PositionAccessor.minValues[2]);
	const glm::vec3 Max(PositionAccessor.maxValues[0], PositionAccessor.maxValues[1], PositionAccessor.maxValues[2]);

	SubmeshBounds.push_back(BoundingBox(Min, Max));
}

void StaticMesh::GLTFLoadMaterial(const std::string& AssetName, AssetManager& Assets, tinygltf::Model& Model, tinygltf::Primitive& Primitive, DRMDevice& Device)
{
	auto& GLTFMaterial = Model.materials[Primitive.material];
	const EMaterialMode MaterialMode = [] (const std::string& AlphaMode)
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
	const std::string MaterialAssetName = AssetName + "_Material_" + std::to_string(Primitive.material);
	const Material* Material = Assets.GetMaterial(MaterialAssetName);

	if (!Material)
	{
		const drm::Image* BaseColor = GLTFLoadImage(Assets, Device, Model, GLTFMaterial.pbrMetallicRoughness.baseColorTexture.index);
		const drm::Image* MetallicRoughness = GLTFLoadImage(Assets, Device, Model, GLTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);
		const drm::Image* Normal = GLTFLoadImage(Assets, Device, Model, GLTFMaterial.normalTexture.index);
		const drm::Image* Emissive = GLTFLoadImage(Assets, Device, Model, GLTFMaterial.emissiveTexture.index);
		const glm::vec3 EmissiveFactor(
			static_cast<float>(GLTFMaterial.emissiveFactor[0]),
			static_cast<float>(GLTFMaterial.emissiveFactor[1]),
			static_cast<float>(GLTFMaterial.emissiveFactor[2])
		);

		if (!BaseColor)
		{
			BaseColor = &AssetManager::Red;
		}

		Material = Assets.LoadMaterial(
			MaterialAssetName,
			std::make_unique<class Material>
			(
				Device,
				MaterialMode,
				BaseColor,
				MetallicRoughness,
				Normal,
				Emissive,
				static_cast<float>(GLTFMaterial.pbrMetallicRoughness.roughnessFactor),
				static_cast<float>(GLTFMaterial.pbrMetallicRoughness.metallicFactor),
				EmissiveFactor
			)
		);
	}

	Materials.push_back(Material);
}

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

const drm::Image* StaticMesh::GLTFLoadImage(AssetManager& Assets, DRMDevice& Device, tinygltf::Model& Model, int32 TextureIndex)
{
	if (TextureIndex == -1)
	{
		return nullptr;
	}
	else
	{
		auto& Texture = Model.textures[TextureIndex];
		auto& Image = Model.images[Texture.source];

		if (const drm::Image* LoadedImage = Assets.GetImage(Image.uri); LoadedImage != nullptr)
		{
			return LoadedImage;
		}
		else
		{
#undef LoadImage
			std::unique_ptr<drm::Image> NewImage = std::make_unique<drm::Image>(Device.CreateImage(
				Image.width, 
				Image.height, 
				1, 
				GetFormat(Image.bits, Image.component, Image.pixel_type), 
				EImageUsage::Sampled | EImageUsage::TransferDst));
			drm::UploadImageData(Device, Image.image.data(), *NewImage);
			return Assets.LoadImage(Image.uri, std::move(NewImage));
		}
	}
}
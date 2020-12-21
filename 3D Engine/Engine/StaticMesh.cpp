#include "StaticMesh.h"
#include "AssetManager.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <tiny_gltf.h>

StaticMesh::StaticMesh(const std::string& assetName, AssetManager& assets, gpu::Device& device, const std::filesystem::path& path)
	: _Name(assetName)
	, _Path(path)
{
	if (_Path.extension() == ".gltf" || _Path.extension() == ".glb")
	{
		GLTFLoad(assetName, assets, device);
	}
	else
	{
		signal_unimplemented();
	}

	std::for_each(_SubmeshBounds.begin(), _SubmeshBounds.end(), [&] (const BoundingBox& submeshBounds)
	{
		_Bounds.TestPoint(submeshBounds.GetMin());
		_Bounds.TestPoint(submeshBounds.GetMax());
	});
}

StaticMesh::StaticMesh(const std::string& assetName, StaticMesh& staticMesh, uint32 submeshIndex)
	: _Name(assetName)
	, _Path(staticMesh._Path)
	, _Materials{ staticMesh._Materials[submeshIndex] }
	, _SubmeshBounds{ staticMesh._SubmeshBounds[submeshIndex] }
	, _SubmeshNames{ staticMesh._SubmeshNames[submeshIndex] }
	, _Bounds{ staticMesh._SubmeshBounds[submeshIndex] }
{
	_Submeshes.emplace_back(std::move(staticMesh._Submeshes[submeshIndex]));
}

tinygltf::TinyGLTF gLoader;

void StaticMesh::GLTFLoad(const std::string& assetName, AssetManager& assets, gpu::Device& device)
{
	tinygltf::Model model;
	std::string err;
	std::string warn;

	if (_Path.extension() == ".gltf")
	{
		gLoader.LoadASCIIFromFile(&model, &err, &warn, _Path.generic_string());
	}
	else
	{
		gLoader.LoadBinaryFromFile(&model, &err, &warn, _Path.generic_string());
	}

	if (!err.empty())
	{
		LOG("TinyGLTF error: %s", err.c_str());
	}
	if (!warn.empty())
	{
		LOG("TinyGLTF warning: %s", warn.c_str());
	}

	for (auto& mesh : model.meshes)
	{
		for (auto& primitive : mesh.primitives)
		{
			GLTFLoadGeometry(model, mesh, primitive, device);
			GLTFLoadMaterial(assetName, assets, model, primitive, device);
			_SubmeshNames.push_back(mesh.name);
		}
	}
}

void StaticMesh::GLTFLoadGeometry(tinygltf::Model& model, tinygltf::Mesh& Mesh, tinygltf::Primitive& Primitive, gpu::Device& device)
{
	auto& indexAccessor = model.accessors[Primitive.indices];
	auto& positionAccessor = model.accessors[Primitive.attributes["POSITION"]];
	auto& normalAccessor = model.accessors[Primitive.attributes["NORMAL"]];
	auto& uvAccessor = model.accessors[Primitive.attributes["TEXCOORD_0"]];

	auto& indexView = model.bufferViews[indexAccessor.bufferView];
	auto& positionView = model.bufferViews[positionAccessor.bufferView];
	auto& normalView = model.bufferViews[normalAccessor.bufferView];
	auto& uvView = model.bufferViews[uvAccessor.bufferView];

	auto& indexData = model.buffers[indexView.buffer];
	auto& positionData = model.buffers[positionView.buffer];
	auto& normalData = model.buffers[normalView.buffer];
	auto& uvData = model.buffers[uvView.buffer];
	
	gpu::Buffer indexBuffer = device.CreateBuffer(EBufferUsage::Index, EMemoryUsage::GPU_ONLY, indexView.byteLength);
	gpu::Buffer positionBuffer = device.CreateBuffer(EBufferUsage::Vertex, EMemoryUsage::GPU_ONLY, positionView.byteLength);
	gpu::Buffer textureCoordinateBuffer = device.CreateBuffer(EBufferUsage::Vertex, EMemoryUsage::GPU_ONLY, uvView.byteLength);
	gpu::Buffer normalBuffer = device.CreateBuffer(EBufferUsage::Vertex, EMemoryUsage::GPU_ONLY, normalView.byteLength);

	gpu::CommandBuffer cmdBuf = device.CreateCommandBuffer(EQueue::Transfer);

	uint64 srcOffset = 0;

	auto stagingBuffer = cmdBuf.CreateStagingBuffer(indexView.byteLength + positionView.byteLength + uvView.byteLength + normalView.byteLength);

	uint8* data = static_cast<uint8*>(stagingBuffer->GetData());

	Platform::Memcpy(data, indexData.data.data() + indexView.byteOffset, indexView.byteLength);
	cmdBuf.CopyBuffer(*stagingBuffer, indexBuffer, srcOffset, 0, indexView.byteLength);
	data += indexView.byteLength;
	srcOffset += indexView.byteLength;

	Platform::Memcpy(data, positionData.data.data() + positionView.byteOffset, positionView.byteLength);
	cmdBuf.CopyBuffer(*stagingBuffer, positionBuffer, srcOffset, 0, positionView.byteLength);
	data += positionView.byteLength;
	srcOffset += positionView.byteLength;

	Platform::Memcpy(data, uvData.data.data() + uvView.byteOffset, uvView.byteLength);
	cmdBuf.CopyBuffer(*stagingBuffer, textureCoordinateBuffer, srcOffset, 0, uvView.byteLength);
	data += uvView.byteLength;
	srcOffset += uvView.byteLength;

	Platform::Memcpy(data, normalData.data.data() + normalView.byteOffset, normalView.byteLength);
	cmdBuf.CopyBuffer(*stagingBuffer, normalBuffer, srcOffset, 0, normalView.byteLength);
	data += normalView.byteLength;
	srcOffset += normalView.byteLength;

	device.SubmitCommands(cmdBuf);

	_Submeshes.emplace_back(Submesh(
		static_cast<uint32>(indexAccessor.count)
		, tinygltf::GetComponentSizeInBytes(indexAccessor.componentType) == 2 ? EIndexType::UINT16 : EIndexType::UINT32
		, std::move(indexBuffer)
		, std::move(positionBuffer)
		, std::move(textureCoordinateBuffer)
		, std::move(normalBuffer)
	));

	const glm::vec3 min(positionAccessor.minValues[0], positionAccessor.minValues[1], positionAccessor.minValues[2]);
	const glm::vec3 max(positionAccessor.maxValues[0], positionAccessor.maxValues[1], positionAccessor.maxValues[2]);

	_SubmeshBounds.push_back(BoundingBox(min, max));
}

void StaticMesh::GLTFLoadMaterial(const std::string& assetName, AssetManager& assets, tinygltf::Model& model, tinygltf::Primitive& primitive, gpu::Device& device)
{
	auto& gltfMaterial = model.materials[primitive.material];
	const EMaterialMode materialMode = [] (const std::string& alphaMode)
	{
		if (alphaMode == "MASK")
		{
			return EMaterialMode::Masked;
		}
		else
		{
			return EMaterialMode::Opaque;
		}
	}(gltfMaterial.alphaMode);
	const std::string materialAssetName = assetName + "_Material_" + std::to_string(primitive.material);
	const Material* material = assets.GetMaterial(materialAssetName);

	if (!material)
	{
		gpu::Image* baseColor = GLTFLoadImage(assets, device, model, gltfMaterial.pbrMetallicRoughness.baseColorTexture.index);
		gpu::Image* metallicRoughness = GLTFLoadImage(assets, device, model, gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);
		gpu::Image* normal = GLTFLoadImage(assets, device, model, gltfMaterial.normalTexture.index);
		gpu::Image* emissive = GLTFLoadImage(assets, device, model, gltfMaterial.emissiveTexture.index);
		const glm::vec3 emissiveFactor = 
			gltfMaterial.emissiveFactor.empty() ? 
			glm::vec3(0.0) : 
			glm::vec3(static_cast<float>(gltfMaterial.emissiveFactor[0]),
			static_cast<float>(gltfMaterial.emissiveFactor[1]),
			static_cast<float>(gltfMaterial.emissiveFactor[2]));

		if (!baseColor)
		{
			baseColor = &AssetManager::Red;
		}

		material = assets.LoadMaterial(
			materialAssetName,
			std::make_unique<Material>
			(
				device,
				materialMode,
				baseColor,
				metallicRoughness,
				normal,
				emissive,
				static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor),
				static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor),
				emissiveFactor
			)
		);
	}

	_Materials.push_back(material);
}

static EFormat GetFormat(int32 bits, int32 components, int32 pixelType)
{
	if (pixelType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE && bits == 8 && components == 4)
	{
		return EFormat::R8G8B8A8_UNORM;
	}
	else
	{
		signal_unimplemented();
	}
}

gpu::Image* StaticMesh::GLTFLoadImage(AssetManager& assets, gpu::Device& device, tinygltf::Model& model, int32 textureIndex)
{
	if (textureIndex == -1 || model.textures.empty())
	{
		return nullptr;
	}
	else
	{
		auto& texture = model.textures[textureIndex];
		auto& image = model.images[texture.source];

		if (gpu::Image* loadedImage = assets.GetImage(image.uri); loadedImage != nullptr)
		{
			return loadedImage;
		}
		else
		{
#undef LoadImage
			std::unique_ptr<gpu::Image> NewImage = std::make_unique<gpu::Image>(device.CreateImage(
				image.width, 
				image.height,
				1, 
				GetFormat(image.bits, image.component, image.pixel_type),
				EImageUsage::Sampled | EImageUsage::TransferDst));
			gpu::UploadImageData(device, image.image.data(), *NewImage);
			return assets.LoadImage(image.uri, std::move(NewImage));
		}
	}
}
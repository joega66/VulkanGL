#include "AssetManager.h"
#include <GPU/GPU.h>

AssetManager::AssetManager(gpu::Device& device)
	: _Device(device)
{
	CreateDebugImages();

	LoadStaticMesh("../Assets/Meshes/Cube/glTF/Cube.gltf");
}

std::vector<const StaticMesh*> AssetManager::LoadStaticMesh(const std::filesystem::path& path, bool breakup)
{
	const std::string assetName = path.stem().generic_string();

	std::unique_ptr<StaticMesh> staticMesh = std::make_unique<StaticMesh>(assetName, *this, _Device, path);

	if (breakup)
	{
		std::vector<const StaticMesh*> outStaticMeshes;

		outStaticMeshes.reserve(staticMesh->_Submeshes.size());

		for (uint32 submeshIndex = 0; submeshIndex < staticMesh->_Submeshes.size(); submeshIndex++)
		{
			const std::string madeUpAssetName = assetName + std::to_string(submeshIndex);

			_StaticMeshes[madeUpAssetName] = std::make_unique<StaticMesh>(madeUpAssetName, *staticMesh, submeshIndex);

			outStaticMeshes.push_back(_StaticMeshes[madeUpAssetName].get());
		}
		return outStaticMeshes;
	}
	else
	{
		_StaticMeshes[assetName] = std::move(staticMesh);

		return { _StaticMeshes[assetName].get() };
	}
}

const StaticMesh* AssetManager::GetStaticMesh(const std::string& assetName) const
{
	return _StaticMeshes.at(assetName).get();
}

gpu::Image* AssetManager::LoadImage(const std::string& assetName, const std::filesystem::path& path, EFormat format, EImageUsage additionalUsage)
{
	check(_Images.find(assetName) == _Images.end(), "Image %s already exists.", assetName.c_str());

	int32 width, height, channels;
	uint8* pixels = Platform::LoadImage(path.generic_string(), width, height, channels);

	if (pixels == nullptr)
	{
		LOG("Warning: Failed to load image %s at %s", assetName.c_str(), path.c_str());
		return nullptr;
	}

	std::unique_ptr<gpu::Image> image = std::make_unique<gpu::Image>(
		_Device.CreateImage(width, height, 1, format, EImageUsage::Sampled | EImageUsage::TransferDst | additionalUsage)
	);

	gpu::UploadImageData(_Device, pixels, *image);

	Platform::FreeImage(pixels);

	_Images[assetName] = std::move(image);

	return _Images[assetName].get();
}

gpu::Image* AssetManager::LoadImage(const std::filesystem::path& path, std::unique_ptr<gpu::Image> image)
{
	check(_Images.find(path.generic_string()) == _Images.end(), "Image %s already exists.", path.generic_string().c_str());

	_Images[path.generic_string()] = std::move(image);

	return _Images[path.generic_string()].get();
}

gpu::Image* AssetManager::GetImage(const std::string& assetName) const
{
	return _Images.find(assetName) == _Images.end() ? nullptr : _Images.at(assetName).get();
}

const Material* AssetManager::LoadMaterial(const std::string& assetName, std::unique_ptr<Material> material)
{
	check(_Materials.find(assetName) == _Materials.end(), "Material %s already exists.", assetName.c_str());

	_Materials[assetName] = std::move(material);

	return _Materials[assetName].get();
}

const Material* AssetManager::GetMaterial(const std::string& assetName)
{
	return _Materials.find(assetName) == _Materials.end() ? nullptr : _Materials[assetName].get();
}

Skybox* AssetManager::LoadSkybox(const std::string& assetName, const std::filesystem::path& path)
{
	check(_Skyboxes.find(assetName) == _Skyboxes.end(), "Skybox %s already exists.", assetName.c_str());

	if (path.has_extension())
	{
		signal_unimplemented();
	}
	else
	{
		const std::string pathStr = path.string();

		std::array<gpu::Image*, 6> images;

		for (uint32 face = CubemapFace_Begin; face != CubemapFace_End; face++)
		{
			const std::string& stem = Skybox::_CubemapStems[face];

			const std::string imageName = assetName + "_" + stem;

			gpu::Image* image = GetImage(imageName);

			if (!image)
			{
				const std::filesystem::path imagePath = pathStr + stem + ".png";

				image = LoadImage(imageName, imagePath, EFormat::R8G8B8A8_UNORM, EImageUsage::TransferSrc);

				if (!image)
				{
					fail("Skybox %s is missing a %s image",
						assetName.c_str(),
						Skybox::_CubemapFaces[face].c_str()
					);
				}
			}

			images[face] = image;
		}

		_Skyboxes[assetName] = std::make_unique<Skybox>(_Device, images, EFormat::R8G8B8A8_UNORM);

		return _Skyboxes[assetName].get();
	}
}

Skybox* AssetManager::GetSkybox(const std::string& assetName)
{
	return _Skyboxes[assetName].get();
}

gpu::Image AssetManager::_Red;
gpu::Image AssetManager::_Green;
gpu::Image AssetManager::_Blue;
gpu::Image AssetManager::_White;
gpu::Image AssetManager::_Black;

void AssetManager::CreateDebugImages() const
{
	const std::vector<uint8> colors =
	{
		255, 0, 0, 0, // Red
		0, 255, 0, 0, // Green
		0, 0, 255, 0, // Blue
		255, 255, 255, 0, // White
		0, 0, 0, 0, // Black
	};

	gpu::CommandBuffer cmdBuf = _Device.CreateCommandBuffer(EQueue::Transfer);

	auto stagingBuffer = cmdBuf.CreateStagingBuffer(colors.size(), colors.data());

	AssetManager::_Red = _Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::_Green = _Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::_Blue = _Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::_White = _Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::_Black = _Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);

	std::vector<ImageMemoryBarrier> barriers
	{
		ImageMemoryBarrier{ AssetManager::_Red, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
		ImageMemoryBarrier{ AssetManager::_Green, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
		ImageMemoryBarrier{ AssetManager::_Blue, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
		ImageMemoryBarrier{ AssetManager::_White, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
		ImageMemoryBarrier{ AssetManager::_Black, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
	};

	cmdBuf.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, barriers.size(), barriers.data());

	for (uint32 colorIndex = 0; colorIndex < barriers.size(); colorIndex++)
	{
		cmdBuf.CopyBufferToImage(*stagingBuffer, colorIndex * 4 * sizeof(uint8), barriers[colorIndex].image, EImageLayout::TransferDstOptimal);
	}

	for (auto& barrier : barriers)
	{
		barrier.srcAccessMask = EAccess::TransferWrite;
		barrier.dstAccessMask = EAccess::MemoryRead;
		barrier.oldLayout = EImageLayout::TransferDstOptimal;
		barrier.newLayout = EImageLayout::ShaderReadOnlyOptimal;
	}

	cmdBuf.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, barriers.size(), barriers.data());

	_Device.SubmitCommands(cmdBuf);
}
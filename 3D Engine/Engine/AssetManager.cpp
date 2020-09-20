#include "AssetManager.h"
#include <GPU/GPU.h>

AssetManager::AssetManager(gpu::Device& Device)
	: Device(Device)
{
	CreateDebugImages(Device);

	LoadStaticMesh("../Assets/Meshes/Cube/glTF/Cube.gltf");
}

std::vector<const StaticMesh*> AssetManager::LoadStaticMesh(const std::filesystem::path& Path, bool Breakup)
{
	const std::string AssetName = Path.stem().generic_string();

	std::unique_ptr<StaticMesh> StaticMesh = std::make_unique<class StaticMesh>(AssetName, *this, Device, Path);

	if (Breakup)
	{
		std::vector<const class StaticMesh*> OutStaticMeshes;
		OutStaticMeshes.reserve(StaticMesh->Submeshes.size());
		for (uint32 SubmeshIndex = 0; SubmeshIndex < StaticMesh->Submeshes.size(); SubmeshIndex++)
		{
			const std::string MadeUpAssetName = AssetName + std::to_string(SubmeshIndex);
			StaticMeshes[MadeUpAssetName] = std::make_unique<class StaticMesh>(MadeUpAssetName, *StaticMesh, SubmeshIndex);
			OutStaticMeshes.push_back(StaticMeshes[MadeUpAssetName].get());
		}
		return OutStaticMeshes;
	}
	else
	{
		StaticMeshes[AssetName] = std::move(StaticMesh);
		return { StaticMeshes[AssetName].get() };
	}
}

const StaticMesh* AssetManager::GetStaticMesh(const std::string& AssetName) const
{
	return StaticMeshes.at(AssetName).get();
}

const gpu::Image* AssetManager::LoadImage(const std::string& AssetName, const std::filesystem::path& Path, EFormat Format, EImageUsage AdditionalUsage)
{
	check(Images.find(AssetName) == Images.end(), "Image %s already exists.", AssetName.c_str());

	int32 Width, Height, Channels;
	uint8* Pixels = Platform::LoadImage(Path.generic_string(), Width, Height, Channels);

	if (Pixels == nullptr)
	{
		LOG("Warning: Failed to load image %s at %s", AssetName.c_str(), Path.c_str());
		return nullptr;
	}

	std::unique_ptr<gpu::Image> Image = std::make_unique<gpu::Image>(
		Device.CreateImage(Width, Height, 1, Format, EImageUsage::Sampled | EImageUsage::TransferDst | AdditionalUsage)
	);

	gpu::UploadImageData(Device, Pixels, *Image);

	Platform::FreeImage(Pixels);

	Images[AssetName] = std::move(Image);

	return Images[AssetName].get();
}

const gpu::Image* AssetManager::LoadImage(const std::filesystem::path& Path, std::unique_ptr<gpu::Image> Image)
{
	check(Images.find(Path.generic_string()) == Images.end(), "Image %s already exists.", Path.generic_string().c_str());
	Images[Path.generic_string()] = std::move(Image);
	return Images[Path.generic_string()].get();
}

const gpu::Image* AssetManager::GetImage(const std::string& AssetName) const
{
	return Images.find(AssetName) == Images.end() ? nullptr : Images.at(AssetName).get();
}

const Material* AssetManager::LoadMaterial(const std::string& AssetName, std::unique_ptr<Material> Material)
{
	check(Materials.find(AssetName) == Materials.end(), "Material %s already exists.", AssetName.c_str());

	Materials[AssetName] = std::move(Material);

	return Materials[AssetName].get();
}

const Material* AssetManager::GetMaterial(const std::string& AssetName)
{
	return Materials.find(AssetName) == Materials.end() ? nullptr : Materials[AssetName].get();
}

Skybox* AssetManager::LoadSkybox(const std::string& AssetName, const std::filesystem::path& Path)
{
	check(Skyboxes.find(AssetName) == Skyboxes.end(), "Skybox %s already exists.", AssetName.c_str());

	if (Path.has_extension())
	{
		signal_unimplemented();
	}
	else
	{
		const std::string PathStr = Path.string();

		std::array<const gpu::Image*, 6> Images;

		for (uint32 Face = CubemapFace_Begin; Face != CubemapFace_End; Face++)
		{
			const std::string& Stem = Skybox::CubemapStems[Face];
			const std::string ImageName = AssetName + "_" + Stem;
			const gpu::Image* Image = GetImage(ImageName);

			if (!Image)
			{
				const std::filesystem::path ImagePath = PathStr + Stem + ".png";

				Image = LoadImage(ImageName, ImagePath, EFormat::R8G8B8A8_UNORM, EImageUsage::TransferSrc);

				if (!Image)
				{
					fail("Skybox %s is missing a %s image",
						AssetName.c_str(),
						Skybox::CubemapFaces[Face].c_str()
					);
				}
			}

			Images[Face] = Image;
		}

		Skyboxes[AssetName] = std::make_unique<Skybox>(Device, Images, EFormat::R8G8B8A8_UNORM);

		return Skyboxes[AssetName].get();
	}
}

Skybox* AssetManager::GetSkybox(const std::string& AssetName)
{
	return Skyboxes[AssetName].get();
}

gpu::Image AssetManager::Red;
gpu::Image AssetManager::Green;
gpu::Image AssetManager::Blue;
gpu::Image AssetManager::White;
gpu::Image AssetManager::Black;

void AssetManager::CreateDebugImages(gpu::Device& Device)
{
	std::vector<uint8> Colors =
	{
		255, 0, 0, 0, // Red
		0, 255, 0, 0, // Green
		0, 0, 255, 0, // Blue
		255, 255, 255, 0, // White
		0, 0, 0, 0, // Black
	};

	gpu::Buffer StagingBuffer = Device.CreateBuffer({}, EMemoryUsage::CPU_ONLY, Colors.size(), Colors.data());

	AssetManager::Red = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::Green = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::Blue = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::White = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::Black = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);

	gpu::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

	std::vector<ImageMemoryBarrier> Barriers
	{
		ImageMemoryBarrier{ AssetManager::Red, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
		ImageMemoryBarrier{ AssetManager::Green, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
		ImageMemoryBarrier{ AssetManager::Blue, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
		ImageMemoryBarrier{ AssetManager::White, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
		ImageMemoryBarrier{ AssetManager::Black, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal },
	};

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, Barriers.size(), Barriers.data());

	for (uint32 ColorIndex = 0; ColorIndex < Barriers.size(); ColorIndex++)
	{
		CmdList.CopyBufferToImage(StagingBuffer, ColorIndex * 4 * sizeof(uint8), Barriers[ColorIndex].image, EImageLayout::TransferDstOptimal);
	}

	for (auto& Barrier : Barriers)
	{
		Barrier.srcAccessMask = EAccess::TransferWrite;
		Barrier.dstAccessMask = EAccess::ShaderRead;
		Barrier.oldLayout = EImageLayout::TransferDstOptimal;
		Barrier.newLayout = EImageLayout::ShaderReadOnlyOptimal;
	}

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, Barriers.size(), Barriers.data());

	Device.SubmitCommands(CmdList);
}
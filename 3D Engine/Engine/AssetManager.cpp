#include "AssetManager.h"
#include <DRM.h>

AssetManager::AssetManager(DRMDevice& Device)
	: Device(Device)
{
	CreateDebugImages(Device);

	std::array<std::string, 6> Cubemap =
	{
		"../Images/Cubemaps/DownUnder/down-under_rt.tga", "../Images/Cubemaps/DownUnder/down-under_lf.tga",
		"../Images/Cubemaps/DownUnder/down-under_up.tga", "../Images/Cubemaps/DownUnder/down-under_dn.tga",
		"../Images/Cubemaps/DownUnder/down-under_bk.tga", "../Images/Cubemaps/DownUnder/down-under_ft.tga",
	};

	LoadCubemap("Engine_Cubemap_Default", Cubemap);
	LoadStaticMesh("../Meshes/Primitives/Cube.gltf");

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
			const std::string MadeUpSubmeshName = AssetName + std::to_string(SubmeshIndex);
			StaticMeshes[MadeUpSubmeshName] = std::make_unique<class StaticMesh>(*StaticMesh, SubmeshIndex);
			OutStaticMeshes.push_back(StaticMeshes[MadeUpSubmeshName].get());
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

void AssetManager::LoadImage(const std::string& AssetName, const std::filesystem::path& Path, EFormat Format)
{
	check(Images.find(AssetName) == Images.end(), "Image %s already exists.", AssetName.c_str());

	int32 Width, Height, Channels;
	uint8* Pixels = Platform::LoadImage(Path.generic_string(), Width, Height, Channels);

	std::unique_ptr<drm::Image> Image = std::make_unique<drm::Image>(Device.CreateImage(Width, Height, 1, Format, EImageUsage::Sampled | EImageUsage::TransferDst));

	drm::UploadImageData(Device, Pixels, *Image);

	Platform::FreeImage(Pixels);

	Images[AssetName] = std::move(Image);
}

const drm::Image* AssetManager::LoadImage(const std::filesystem::path& Path, std::unique_ptr<drm::Image> Image)
{
	check(Images.find(Path.generic_string()) == Images.end(), "Image %s already exists.", Path.generic_string().c_str());
	Images[Path.generic_string()] = std::move(Image);
	return Images[Path.generic_string()].get();
}

const drm::Image* AssetManager::GetImage(const std::string& AssetName) const
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

void AssetManager::LoadCubemap(const std::string& AssetName, const std::array<std::string, 6>& Files, EFormat Format)
{
	std::unique_ptr<drm::Image> Image;
	drm::Buffer StagingBuffer;
	void* MemMapped = nullptr;

	for (uint32 FaceIndex = 0; FaceIndex < Files.size(); FaceIndex++)
	{
		int32 Width, Height, Channels;
		uint8* Pixels = Platform::LoadImage(Files[FaceIndex], Width, Height, Channels);

		if (FaceIndex == 0)
		{
			Image = std::make_unique<drm::Image>(Device.CreateImage(Width, Height, 1, Format, EImageUsage::Sampled | EImageUsage::Cubemap | EImageUsage::TransferDst));
			StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, Image->GetSize());
			MemMapped = StagingBuffer.GetData();
		}

		check(Image->GetWidth() == Width && Image->GetHeight() == Height, "Cubemap faces must all be the same size.");

		Platform::Memcpy((uint8*)MemMapped + FaceIndex * (Image->GetSize() / 6), Pixels, (Image->GetSize() / 6));

		Platform::FreeImage(Pixels);
	}

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

	ImageMemoryBarrier Barrier{
		*Image,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	};

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

	CmdList.CopyBufferToImage(StagingBuffer, 0, *Image, EImageLayout::TransferDstOptimal);

	Barrier.SrcAccessMask = EAccess::TransferWrite;
	Barrier.DstAccessMask = EAccess::ShaderRead;
	Barrier.OldLayout = EImageLayout::TransferDstOptimal;
	Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;
	
	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &Barrier);

	Device.SubmitCommands(CmdList);

	Cubemaps[AssetName] = std::move(Image);
}

const drm::Image* AssetManager::GetCubemap(const std::string& AssetName) const
{
	return Cubemaps.at(AssetName).get();
}

drm::Image AssetManager::Red;
drm::Image AssetManager::Green;
drm::Image AssetManager::Blue;
drm::Image AssetManager::White;
drm::Image AssetManager::Black;

void AssetManager::CreateDebugImages(DRMDevice& Device)
{
	std::vector<uint8> Colors =
	{
		255, 0, 0, 0, // Red
		0, 255, 0, 0, // Green
		0, 0, 255, 0, // Blue
		255, 255, 255, 0, // White
		0, 0, 0, 0, // Black
	};

	drm::Buffer StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, Colors.size(), Colors.data());

	AssetManager::Red = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::Green = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::Blue = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::White = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	AssetManager::Black = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

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
		CmdList.CopyBufferToImage(StagingBuffer, ColorIndex * 4 * sizeof(uint8), Barriers[ColorIndex].Image, EImageLayout::TransferDstOptimal);
	}

	for (auto& Barrier : Barriers)
	{
		Barrier.SrcAccessMask = EAccess::TransferWrite;
		Barrier.DstAccessMask = EAccess::ShaderRead;
		Barrier.OldLayout = EImageLayout::TransferDstOptimal;
		Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;
	}

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, Barriers.size(), Barriers.data());

	Device.SubmitCommands(CmdList);
}
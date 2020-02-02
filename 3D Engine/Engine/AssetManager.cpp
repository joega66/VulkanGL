#include "AssetManager.h"
#include <DRM.h>

AssetManager::AssetManager(DRMDevice& Device)
	: Device(Device)
{
	std::array<std::string, 6> Cubemap =
	{
		"../Images/Cubemaps/DownUnder/down-under_rt.tga", "../Images/Cubemaps/DownUnder/down-under_lf.tga",
		"../Images/Cubemaps/DownUnder/down-under_up.tga", "../Images/Cubemaps/DownUnder/down-under_dn.tga",
		"../Images/Cubemaps/DownUnder/down-under_bk.tga", "../Images/Cubemaps/DownUnder/down-under_ft.tga",
	};

	LoadCubemap("Engine_Cubemap_Default", Cubemap);
	LoadImage("Engine_Diffuse_Default", "../Images/Frozen-Ice-Texture.jpg");
	LoadStaticMesh("Cube", "../Meshes/Primitives/Cube.obj");
	LoadStaticMesh("Transform_Gizmo", "../Meshes/Primitives/TransformGizmo/TransformGizmo.obj");
}

std::vector<const StaticMesh*> AssetManager::LoadStaticMesh(const std::string& Name, const std::string& File, bool Breakup)
{
	std::unique_ptr<StaticMesh> StaticMesh = std::make_unique<class StaticMesh>(Device, File);

	// Add default engine materials if missing.
	for (uint32 SubmeshIndex = 0; SubmeshIndex < StaticMesh->Submeshes.size(); SubmeshIndex++)
	{
		if (!StaticMesh->Materials[SubmeshIndex].MaterialSet.Diffuse)
		{
			StaticMesh->Materials[SubmeshIndex].MaterialSet.Diffuse = GetImage("Engine_Diffuse_Default");
		}
	}

	if (Breakup)
	{
		std::vector<const class StaticMesh*> OutStaticMeshes;
		OutStaticMeshes.reserve(StaticMesh->Submeshes.size());
		for (uint32 SubmeshIndex = 0; SubmeshIndex < StaticMesh->Submeshes.size(); SubmeshIndex++)
		{
			const std::string MadeUpSubmeshName = Name + std::to_string(SubmeshIndex);
			StaticMeshes[MadeUpSubmeshName] = std::make_unique<class StaticMesh>(*StaticMesh, SubmeshIndex);
			OutStaticMeshes.push_back(StaticMeshes[MadeUpSubmeshName].get());
		}
		return OutStaticMeshes;
	}
	else
	{
		StaticMeshes[Name] = std::move(StaticMesh);
		return { StaticMeshes[Name].get() };
	}
}

const StaticMesh* AssetManager::GetStaticMesh(const std::string& Name) const
{
	return GetValue(StaticMeshes, Name).get();
}

void AssetManager::LoadImage(const std::string& Name, const std::string& File, EFormat Format)
{
	int32 Width, Height, Channels;
	uint8* Pixels = Platform::LoadImage(File, Width, Height, Channels);

	drm::ImageRef Image = Device.CreateImage(Width, Height, 1, Format, EImageUsage::Sampled | EImageUsage::TransferDst);

	{
		drm::CommandListRef CmdList = Device.CreateCommandList();

		drm::BufferRef StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, Image->GetSize(), Pixels);

		ImageMemoryBarrier Barrier(
			Image,
			EAccess::None,
			EAccess::TransferWrite,
			EImageLayout::Undefined,
			EImageLayout::TransferDstOptimal
		);

		CmdList->PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

		CmdList->CopyBufferToImage(StagingBuffer, 0, Image, EImageLayout::TransferDstOptimal);

		Barrier.SrcAccessMask = EAccess::TransferWrite;
		Barrier.DstAccessMask = EAccess::ShaderRead;
		Barrier.OldLayout = EImageLayout::TransferDstOptimal;
		Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;

		CmdList->PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &Barrier);

		Device.SubmitCommands(CmdList);
	}
	
	Platform::FreeImage(Pixels);

	Images[Name] = Image;
}

drm::ImageRef AssetManager::GetImage(const std::string& Name) const
{
	return GetValue(Images, Name);
}

void AssetManager::LoadCubemap(const std::string& Name, const std::array<std::string, 6>& Files, EFormat Format)
{
	drm::ImageRef Image;
	drm::BufferRef StagingBuffer;
	void* MemMapped = nullptr;

	for (uint32 FaceIndex = 0; FaceIndex < Files.size(); FaceIndex++)
	{
		int32 Width, Height, Channels;
		uint8* Pixels = Platform::LoadImage(Files[FaceIndex], Width, Height, Channels);

		if (FaceIndex == 0)
		{
			Image = Device.CreateImage(Width, Height, 1, Format, EImageUsage::Sampled | EImageUsage::Cubemap | EImageUsage::TransferDst);
			StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, Image->GetSize());
			MemMapped = Device.LockBuffer(StagingBuffer);
		}

		check(Image->Width == Width && Image->Height == Height, "Cubemap faces must all be the same size.");

		Platform::Memcpy((uint8*)MemMapped + FaceIndex * (Image->GetSize() / 6), Pixels, (Image->GetSize() / 6));

		Platform::FreeImage(Pixels);
	}

	Device.UnlockBuffer(StagingBuffer);

	drm::CommandListRef CmdList = Device.CreateCommandList();

	ImageMemoryBarrier Barrier(
		Image,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	);

	CmdList->PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

	CmdList->CopyBufferToImage(StagingBuffer, 0, Image, EImageLayout::TransferDstOptimal);

	Barrier.SrcAccessMask = EAccess::TransferWrite;
	Barrier.DstAccessMask = EAccess::ShaderRead;
	Barrier.OldLayout = EImageLayout::TransferDstOptimal;
	Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;
	
	CmdList->PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &Barrier);

	Device.SubmitCommands(CmdList);

	Cubemaps[Name] = Image;
}

drm::ImageRef AssetManager::GetCubemap(const std::string& Name) const
{
	return GetValue(Cubemaps, Name);
}
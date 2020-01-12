#include "AssetManager.h"
#include "../DRM.h"

AssetManager::AssetManager()
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
	std::unique_ptr<StaticMesh> StaticMesh = std::make_unique<class StaticMesh>(File);

	// Add default engine materials if missing.
	for (uint32 SubmeshIndex = 0; SubmeshIndex < StaticMesh->Submeshes.size(); SubmeshIndex++)
	{
		if (!StaticMesh->Materials[SubmeshIndex].Diffuse)
		{
			StaticMesh->Materials[SubmeshIndex].Diffuse = GetImage("Engine_Diffuse_Default");
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
	uint8* Pixels = Platform.LoadImage(File, Width, Height, Channels);
	Images[Name] = drm::CreateImage(Width, Height, 1, Format, EImageUsage::Sampled, Pixels);
	Platform.FreeImage(Pixels);
}

drm::ImageRef AssetManager::GetImage(const std::string& Name) const
{
	return GetValue(Images, Name);
}

void AssetManager::LoadCubemap(const std::string& Name, const std::array<std::string, 6>& Files, EFormat Format)
{
	CubemapCreateInfo CubemapCreateInfo;

	for (uint32 i = 0; i < Files.size(); i++)
	{
		auto& File = Files[i];
		auto& Face = CubemapCreateInfo.CubeFaces[i];
		int32 Width, Height, Channels;
		uint8* Pixels = Platform.LoadImage(File, Width, Height, Channels);
		Face = { Width, Height, Pixels };
	}

	auto& Face = CubemapCreateInfo.CubeFaces[0];

	check(std::all_of(CubemapCreateInfo.CubeFaces.begin() + 1, CubemapCreateInfo.CubeFaces.end(), 
		[&](const auto& Other)
	{
		return Face.Width == Other.Width && Face.Height == Other.Height;
	}), "Cubemap faces must have same dimensions.");

	Cubemaps[Name] = drm::CreateCubemap(
		Face.Width
		, Face.Height
		, Format
		, EImageUsage::Sampled
		, CubemapCreateInfo);

	std::for_each(CubemapCreateInfo.CubeFaces.begin(), CubemapCreateInfo.CubeFaces.end(), [&](const auto& Other)
	{
		Platform.FreeImage(Other.Data);
	});
}

drm::ImageRef AssetManager::GetCubemap(const std::string& Name) const
{
	return GetValue(Cubemaps, Name);
}
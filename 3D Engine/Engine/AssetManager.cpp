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

StaticMeshRef AssetManager::LoadStaticMesh(const std::string& Name, const std::string& File)
{
	StaticMeshRef Static = MakeRef<StaticMesh>(File);

	// Add default engine materials if missing.
	for (auto& Element : Static->Batch.Elements)
	{
		if (!Element.Material.Diffuse)
		{
			Element.Material.Diffuse = GetImage("Engine_Diffuse_Default");
		}
	}

	StaticMeshes[Name] = Static;

	return Static;
}

StaticMeshRef AssetManager::GetStaticMesh(const std::string& Name) const
{
	return GetValue(StaticMeshes, Name);
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
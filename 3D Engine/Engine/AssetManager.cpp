#include "AssetManager.h"
#include "../DRM.h"

AssetManager GAssetManager;

StaticMeshRef AssetManager::LoadStaticMesh(const std::string& Name, const std::string& File)
{
	StaticMeshRef Static = MakeRef<StaticMesh>(File);
	StaticMeshes[Name] = Static;
	return Static;
}

StaticMeshRef AssetManager::GetStaticMesh(const std::string & Name)
{
	return GetValue(StaticMeshes, Name);
}

void AssetManager::LoadImage(const std::string& Name, const std::string& File, EImageFormat Format)
{
	int32 Width, Height, Channels;
	uint8* Pixels = GPlatform->LoadImage(File, Width, Height, Channels);
	Images[Name] = GLCreateImage(Width, Height, Format, EResourceUsage::ShaderResource, Pixels);
	GPlatform->FreeImage(Pixels);
}

GLImageRef AssetManager::GetImage(const std::string& Name)
{
	return GetValue(Images, Name);
}

void AssetManager::LoadCubemap(const std::string& Name, const std::array<std::string, 6>& Files, EImageFormat Format)
{
	CubemapCreateInfo CubemapCreateInfo;

	for (uint32 i = 0; i < Files.size(); i++)
	{
		auto& File = Files[i];
		auto& Face = CubemapCreateInfo.CubeFaces[i];
		int32 Width, Height, Channels;
		uint8* Pixels = GPlatform->LoadImage(File, Width, Height, Channels);
		Face = { Width, Height, Pixels };
	}

	auto& Face = CubemapCreateInfo.CubeFaces[0];

	check(std::all_of(CubemapCreateInfo.CubeFaces.begin() + 1, CubemapCreateInfo.CubeFaces.end(), 
		[&](const auto& Other)
	{
		return Face.Width == Other.Width && Face.Height == Other.Height;
	}), "Cubemap faces must have same dimensions.");

	Cubemaps[Name] = GLCreateCubemap(
		Face.Width
		, Face.Height
		, Format
		, EResourceUsage::ShaderResource
		, CubemapCreateInfo);

	std::for_each(CubemapCreateInfo.CubeFaces.begin(), CubemapCreateInfo.CubeFaces.end(), [&](const auto& Other)
	{
		GPlatform->FreeImage(Other.Data);
	});
}

GLImageRef AssetManager::GetCubemap(const std::string& Name)
{
	return Cubemaps[Name];
}
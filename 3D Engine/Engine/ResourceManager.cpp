#include "ResourceManager.h"
#include "../GL.h"

AssetManager GAssetManager;

StaticMeshRef AssetManager::SaveStaticMesh(const std::string& Name, const std::string& File)
{
	StaticMeshRef Static = MakeRef<StaticMesh>(File);
	StaticMeshes[Name] = Static;
	return Static;
}

StaticMeshRef AssetManager::GetStaticMesh(const std::string & Name)
{
	return GetValue(StaticMeshes, Name);
}

void AssetManager::SaveImage(const std::string& Name, const std::string& File, EImageFormat Format)
{
	int32 Width, Height, Channels;
	uint8* Pixels = GPlatform->LoadImage(File, Width, Height, Channels);
	Images[Name] = GLCreateImage(Width, Height, Format, RU_ShaderResource, Pixels);
	GPlatform->FreeImage(Pixels);
}

GLImageRef AssetManager::GetImage(const std::string& Name)
{
	return GetValue(Images, Name);
}
#pragma once
#include "StaticMesh.h"

class AssetManager
{
public:
	AssetManager() = default;

	StaticMeshRef LoadStaticMesh(const std::string& Name, const std::string& File);
	StaticMeshRef GetStaticMesh(const std::string& Name);

	void LoadImage(const std::string& Name, const std::string& File, EImageFormat Format = IF_R8G8B8A8_UNORM);
	GLImageRef GetImage(const std::string& Name);

	void LoadCubemap(const std::string& Name, const std::array<std::string, 6>& Files, EImageFormat Format = IF_R8G8B8A8_UNORM);
	GLImageRef GetCubemap(const std::string& Name);

private:
	HashTable<std::string, StaticMeshRef> StaticMeshes;
	HashTable<std::string, GLImageRef> Images;
	HashTable<std::string, GLImageRef> Cubemaps;
};

extern AssetManager GAssetManager;
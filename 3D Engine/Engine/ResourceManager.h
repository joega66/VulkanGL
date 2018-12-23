#pragma once
#include "StaticMesh.h"

class AssetManager
{
public:
	AssetManager() = default;

	StaticMeshRef SaveStaticMesh(const std::string& Name, const std::string& File);
	StaticMeshRef GetStaticMesh(const std::string& Name);

	void SaveImage(const std::string& Name, const std::string& File, EImageFormat Format = IF_R8G8B8A8_UNORM);
	GLImageRef GetImage(const std::string& Name);

private:
	Map<std::string, StaticMeshRef> StaticMeshes;
	Map<std::string, GLImageRef> Images;
};

extern AssetManager GAssetManager;
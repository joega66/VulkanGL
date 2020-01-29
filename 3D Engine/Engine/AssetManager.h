#pragma once
#include "StaticMesh.h"

class DRMDevice;

class AssetManager
{
	friend class Scene;
	AssetManager(DRMDevice& Device);

public:
	AssetManager(const AssetManager&) = delete;
	AssetManager& operator=(const AssetManager&) = delete;

	/**
	 * @param Name Name of mesh asset.
	 * @param File Filename to load the mesh from.
	 * @param Breakup Whether to divide the submeshes into their own meshes.
	 * @return Array of static mesh render resources. Size is 1 if Breakup is false.
	 * @TODO Replace with std::span
	*/
	std::vector<const StaticMesh*> LoadStaticMesh(const std::string& Name, const std::string& File, bool Breakup = false);
	const StaticMesh* GetStaticMesh(const std::string& Name) const;

	void LoadImage(const std::string& Name, const std::string& File, EFormat Format = EFormat::R8G8B8A8_UNORM);
	drm::ImageRef GetImage(const std::string& Name) const;

	void LoadCubemap(const std::string& Name, const std::array<std::string, 6>& Files, EFormat Format = EFormat::R8G8B8A8_UNORM);
	drm::ImageRef GetCubemap(const std::string& Name) const;

private:
	DRMDevice& Device;

	HashTable<std::string, std::unique_ptr<StaticMesh>> StaticMeshes;
	HashTable<std::string, drm::ImageRef> Images;
	HashTable<std::string, drm::ImageRef> Cubemaps;
};
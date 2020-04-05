#pragma once
#include "StaticMesh.h"
#include <filesystem>

class DRMDevice;

class AssetManager
{
	friend class Engine;
	AssetManager(DRMDevice& Device);

public:
	AssetManager(const AssetManager&) = delete;
	AssetManager& operator=(const AssetManager&) = delete;

	/**
	 * @param Name Name of mesh asset.
	 * @param File Filename to load the mesh from.
	 * @param Breakup Whether to divide the submeshes into their own meshes.
	 * @return Array of static mesh render resources. Size is 1 if Breakup is false.
	 * @todo Replace with std::span
	*/
	std::vector<const StaticMesh*> LoadStaticMesh(const std::filesystem::path& Path, bool Breakup = false);
	const StaticMesh* GetStaticMesh(const std::string& AssetName) const;

	void LoadImage(const std::string& AssetName, const std::filesystem::path& Path, EFormat Format = EFormat::R8G8B8A8_UNORM);
	const drm::Image* LoadImage(const std::filesystem::path& Path, std::unique_ptr<drm::Image> Image);
	const drm::Image* GetImage(const std::string& AssetName) const;

	const Material* LoadMaterial(const std::string& AssetName, std::unique_ptr<Material> Material);
	const Material* GetMaterial(const std::string& AssetName);

	void LoadCubemap(const std::string& AssetName, const std::array<std::string, 6>& Files, EFormat Format = EFormat::R8G8B8A8_UNORM);
	const drm::Image* GetCubemap(const std::string& AssetName) const;

	inline drm::BindlessResources& GetBindlessSampledImages() { return BindlessSampledImages; }

private:
	DRMDevice& Device;

	drm::BindlessResources BindlessSampledImages;

	HashTable<std::string, std::unique_ptr<StaticMesh>> StaticMeshes;
	HashTable<std::string, std::unique_ptr<Material>> Materials;
	HashTable<std::string, std::unique_ptr<drm::Image>> Images;
	HashTable<std::string, std::unique_ptr<drm::Image>> Cubemaps;
};
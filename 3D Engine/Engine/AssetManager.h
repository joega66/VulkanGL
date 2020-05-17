#pragma once
#include "StaticMesh.h"
#include "Skybox.h"
#include <filesystem>

class drm::Device;

class AssetManager
{
	friend class Engine;
	AssetManager(drm::Device& Device);

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

	const drm::Image* LoadImage(const std::string& AssetName, const std::filesystem::path& Path, EFormat Format, EImageUsage AdditionalUsage = EImageUsage::None);
	const drm::Image* LoadImage(const std::filesystem::path& Path, std::unique_ptr<drm::Image> Image);
	const drm::Image* GetImage(const std::string& AssetName) const;

	const Material* LoadMaterial(const std::string& AssetName, std::unique_ptr<Material> Material);
	const Material* GetMaterial(const std::string& AssetName);

	/** 
	  * Load a skybox. If the path has an extension, it's assumed to be pointing to a file in a cubemap format. 
	  * Otherwise, the loader looks for px, nx, py, ny, pz, nz, in the directory.
	  */
	Skybox* LoadSkybox(const std::string& AssetName, const std::filesystem::path& Path);
	Skybox* GetSkybox(const std::string& AssetName);

	static drm::Image Red;
	static drm::Image Green;
	static drm::Image Blue;
	static drm::Image White;
	static drm::Image Black;

private:
	drm::Device& Device;

	std::unordered_map<std::string, std::unique_ptr<StaticMesh>> StaticMeshes;
	std::unordered_map<std::string, std::unique_ptr<Material>> Materials;
	std::unordered_map<std::string, std::unique_ptr<Skybox>> Skyboxes;
	std::unordered_map<std::string, std::unique_ptr<drm::Image>> Images;

	static void CreateDebugImages(drm::Device& Device);
};
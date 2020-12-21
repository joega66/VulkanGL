#pragma once
#include "StaticMesh.h"
#include "Skybox.h"
#include <filesystem>

class gpu::Device;

class AssetManager
{
	friend class Engine;
	AssetManager(gpu::Device& device);

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
	std::vector<const StaticMesh*> LoadStaticMesh(const std::filesystem::path& path, bool breakup = false);
	const StaticMesh* GetStaticMesh(const std::string& assetName) const;

	gpu::Image* LoadImage(const std::string& assetName, const std::filesystem::path& path, EFormat format, EImageUsage additionalUsage = EImageUsage::None);
	gpu::Image* LoadImage(const std::filesystem::path& path, std::unique_ptr<gpu::Image> image);
	gpu::Image* GetImage(const std::string& assetName) const;

	const Material* LoadMaterial(const std::string& assetName, std::unique_ptr<Material> material);
	const Material* GetMaterial(const std::string& assetName);

	/** 
	  * Load a skybox. If the path has an extension, it's assumed to be pointing to a file in a cubemap format. 
	  * Otherwise, the loader looks for px, nx, py, ny, pz, nz, in the directory.
	  */
	Skybox* LoadSkybox(const std::string& assetName, const std::filesystem::path& path);
	Skybox* GetSkybox(const std::string& assetName);

	static gpu::Image _Red;
	static gpu::Image _Green;
	static gpu::Image _Blue;
	static gpu::Image _White;
	static gpu::Image _Black;

private:
	gpu::Device& _Device;

	std::unordered_map<std::string, std::unique_ptr<StaticMesh>> _StaticMeshes;
	std::unordered_map<std::string, std::unique_ptr<Material>> _Materials;
	std::unordered_map<std::string, std::unique_ptr<Skybox>> _Skyboxes;
	std::unordered_map<std::string, std::unique_ptr<gpu::Image>> _Images;

	void CreateDebugImages() const;
};
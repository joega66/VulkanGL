#include "Scene.h"
#include <Engine/AssetManager.h>

Scene::Scene()
{
	Skybox = GAssetManager.GetCubemap("Engine-Cubemap-Default");
}
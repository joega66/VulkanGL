#include "Scene.h"
#include <Engine/AssetManager.h>
#include <DRM.h>

Scene::Scene(DRM& Device, DRMShaderMap& ShaderMap)
	: ShaderMap(ShaderMap), Assets(Device)
{
	Skybox = Assets.GetCubemap("Engine_Cubemap_Default");
}
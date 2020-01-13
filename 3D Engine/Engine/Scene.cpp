#include "Scene.h"
#include <Engine/AssetManager.h>
#include <DRM.h>
#include "Screen.h"

Scene::Scene(DRM& Device, DRMShaderMap& ShaderMap, Screen& Screen)
	: ShaderMap(ShaderMap), Assets(Device), View(Screen)
{
	Skybox = Assets.GetCubemap("Engine_Cubemap_Default");
}
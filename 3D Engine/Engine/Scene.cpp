#include "Scene.h"
#include <Engine/AssetManager.h>

Scene::Scene(DRMShaderMap& ShaderMap)
	: ShaderMap(ShaderMap)
{
	Skybox = Assets.GetCubemap("Engine_Cubemap_Default");
}
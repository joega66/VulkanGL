#include "Scene.h"
#include <Engine/AssetManager.h>

Scene::Scene()
{
	Skybox = Assets.GetCubemap("Engine_Cubemap_Default");
}
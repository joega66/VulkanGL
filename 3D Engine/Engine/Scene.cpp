#include "Scene.h"
#include "Engine.h"
#include "AssetManager.h"

Scene::Scene(Engine& Engine)
{
	Skybox = Engine.Assets.GetCubemap("Engine_Cubemap_Default");
}
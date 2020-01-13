#include "Scene.h"
#include "AssetManager.h"
#include <DRM.h>

Scene::Scene(DRM& Device, DRMShaderMap& ShaderMap, class Cursor& Cursor, class Input& Input, class Screen& Screen)
	: ShaderMap(ShaderMap)
	, Cursor(Cursor)
	, Input(Input)
	, Assets(Device)
	, View(Screen)
{
	Skybox = Assets.GetCubemap("Engine_Cubemap_Default");
}
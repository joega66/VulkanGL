#pragma once
#include <Platform/Platform.h>

class EngineMain
{
public:
	void Main(
		Platform& Platform,
		class Cursor& Cursor,
		class Input& Input,
		class Screen& Screen,
		class DRM& Device, 
		class DRMShaderMap& ShaderMap
	);
};
#pragma once
#include <Platform/Platform.h>

/** The main() of the engine, after the platform-dependent stuff has been resolved. */
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
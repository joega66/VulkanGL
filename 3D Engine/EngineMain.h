#pragma once
#include <Platform/Platform.h>
#include <DRM.h>

/** The main() of the engine, after the platform-dependent stuff has been resolved. */
class EngineMain
{
public:
	void Main(
		Platform& Platform,
		class Cursor& Cursor,
		class Input& Input,
		class Screen& Screen,
		DRMDevice& Device, 
		class DRMShaderMap& ShaderMap,
		drm::Surface& Surface
	);
};
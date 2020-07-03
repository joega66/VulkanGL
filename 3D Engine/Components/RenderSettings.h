#pragma once
#include <ECS/Component.h>

class RenderSettings : public Component
{
public:
	/** Camera */
	float ExposureAdjustment;
	float ExposureBias;

	/** Ray Tracing */
	bool bRayTracing = false;

	RenderSettings()
		: ExposureAdjustment(Platform::GetFloat("Engine.ini", "Camera", "ExposureAdjustment", 2.0f))
		, ExposureBias(Platform::GetFloat("Engine.ini", "Camera", "ExposureBias", 2.0f))
		, bRayTracing(Platform::GetBool("Engine.ini", "Scene", "RayTracing", false))
		
	{
	}
};
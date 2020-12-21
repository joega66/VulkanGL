#pragma once
#include <ECS/Component.h>

class RenderSettings : public Component
{
public:
	/** Camera */
	float _ExposureAdjustment;
	float _ExposureBias;

	/** Ray Tracing */
	bool _UseRayTracing = false;

	RenderSettings()
		: _ExposureAdjustment(Platform::GetFloat("Engine.ini", "Camera", "ExposureAdjustment", 2.0f))
		, _ExposureBias(Platform::GetFloat("Engine.ini", "Camera", "ExposureBias", 2.0f))
		, _UseRayTracing(Platform::GetBool("Engine.ini", "Scene", "RayTracing", false))
	{
	}
};
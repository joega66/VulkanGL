#pragma once
#include <ECS/Component.h>

class RenderSettings : public Component
{
public:
	/** Whether the scene should be voxelized this frame. */
	bool bVoxelize = true;
	/** Whether the voxels should be visualized. */
	bool bDrawVoxels = false;

	/** Camera */
	float ExposureAdjustment;
	float ExposureBias;

	RenderSettings()
		: ExposureAdjustment( Platform::GetFloat( "Engine.ini", "Camera", "ExposureAdjustment", 2.0f ) )
		, ExposureBias( Platform::GetFloat( "Engine.ini", "Camera", "ExposureBias", 2.0f ) )
	{
	}
};
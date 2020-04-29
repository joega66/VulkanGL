#pragma once
#include <ECS/Component.h>
#include <Renderer/Voxels.h>

class RenderSettings : public Component
{
public:
	EVoxelDebugMode VoxelDebugMode = EVoxelDebugMode::None;

	/** Whether the scene should be voxelized this frame. */
	bool bVoxelize = true;

	/** Camera */
	float ExposureAdjustment;
	float ExposureBias;

	RenderSettings()
		: ExposureAdjustment( Platform::GetFloat( "Engine.ini", "Camera", "ExposureAdjustment", 2.0f ) )
		, ExposureBias( Platform::GetFloat( "Engine.ini", "Camera", "ExposureBias", 2.0f ) )
	{
	}
};
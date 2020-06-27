#pragma once
#include <ECS/Component.h>
#include <Renderer/Voxels.h>

class RenderSettings : public Component
{
public:
	/** Voxel Cone Tracing */
	float VoxelSize;
	glm::vec3 VoxelFieldCenter;
	EVoxelDebugMode VoxelDebugMode = EVoxelDebugMode::None;
	bool bVoxelize = true;

	/** Camera */
	float ExposureAdjustment;
	float ExposureBias;

	/** Ray Tracing */
	bool bRayTracing = true;

	RenderSettings()
		: VoxelFieldCenter(
			Platform::GetFloat("Engine.ini", "Voxels", "VoxelFieldCenterX", 0.0f),
			Platform::GetFloat("Engine.ini", "Voxels", "VoxelFieldCenterY", 0.0f),
			Platform::GetFloat("Engine.ini", "Voxels", "VoxelFieldCenterZ", 0.0f))
		, VoxelSize(Platform::GetFloat("Engine.ini", "Voxels", "VoxelSize", 2.5f))
		, ExposureAdjustment(Platform::GetFloat("Engine.ini", "Camera", "ExposureAdjustment", 2.0f))
		, ExposureBias(Platform::GetFloat("Engine.ini", "Camera", "ExposureBias", 2.0f))
		
	{
	}
};
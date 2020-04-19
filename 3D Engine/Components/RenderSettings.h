#pragma once
#include <ECS/Component.h>

class RenderSettings : public Component
{
public:
	/** Whether the scene should be voxelized this frame. */
	bool bVoxelize = true;
	/** Whether the voxels should be visualized. */
	bool bDrawVoxels = false;
};
#include "Common.glsl"

#ifndef VOXEL_GRID_SIZE
#define VOXEL_GRID_SIZE 512
#endif

#define HALF_VOXEL_GRID_SIZE (1.0f / VOXEL_GRID_SIZE)

layout(binding = 0, set = 2) uniform VoxelOrthoProjBuffer
{
	mat4 VoxelOrthoProj;
};

layout(binding = 1, set = 2, std430) writeonly buffer VoxelColorBuffer
{
	uint VoxelColors[];
};

layout(binding = 2, set = 2, std430) writeonly buffer VoxelPositionBuffer
{
	vec3 VoxelPositions[];
};

layout(binding = 3, set = 2, std430) writeonly buffer VoxelDrawIndirectBuffer
{
	DrawIndirectCommand VoxelDrawIndirect;
};
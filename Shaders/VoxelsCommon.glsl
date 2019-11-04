#ifndef VOXEL_GRID_SIZE
#define VOXEL_GRID_SIZE 512
#endif

#define HALF_VOXEL_GRID_SIZE (1.0f / VOXEL_GRID_SIZE)

layout(binding = 0, set = 2) uniform VoxelOrthoProjBuffer
{
	mat4 Transform;
} VoxelOrthoProj;

//layout(binding = 1, set = 2, rgba8) writeonly uniform image3D VoxelColor;

layout(binding = 1, set = 2, std430) writeonly buffer VoxelColorBuffer
{
	uint NumVoxels;
	uint Data[];
} VoxelColors;

layout(binding = 2, set = 2, std430) writeonly buffer VoxelPositionBuffer
{
	vec3 Data[];
} VoxelPositions;
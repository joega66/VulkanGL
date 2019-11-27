#include "Common.glsl"

#ifndef VOXEL_GRID_SIZE
#define VOXEL_GRID_SIZE 512
#endif

layout(binding = 0, set = VOXEL_SET) uniform WorldToVoxelBuffer
{
	mat4 WorldToVoxel;
	mat4 WorldToVoxelInv;
};

layout(binding = 1, set = VOXEL_SET, rgba8) uniform image3D VoxelDiffuseGI;

layout(binding = 2, set = VOXEL_SET, std430) buffer VoxelPositionBuffer
{
	ivec3 VoxelPositions[];
};

layout(binding = 3, set = VOXEL_SET, std430) buffer VoxelDrawIndirectBuffer
{
	DrawIndirectCommand VoxelDrawIndirect;
};

vec3 TransformWorldToVoxel(vec3 WorldPosition)
{
	vec4 VoxelPosition = WorldToVoxel * vec4(WorldPosition.xyz, 1);
	return VoxelPosition.xyz;
}

vec3 TransformVoxelToWorld(vec3 VoxelPosition)
{
	vec3 WorldPosition = VoxelPosition;
	WorldPosition /= VOXEL_GRID_SIZE;
	WorldPosition.xy = (WorldPosition.xy - 0.5f) * 2.0f;
	WorldPosition = vec3(WorldToVoxelInv * vec4(WorldPosition, 1));
	return WorldPosition;
}
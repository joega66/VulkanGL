#ifndef VOXELS_COMMON
#define VOXELS_COMMON
#include "Common.glsl"

#ifndef VOXEL_GRID_SIZE
#define VOXEL_GRID_SIZE 512
#endif

const float VOXEL_GRID_SIZE_INV = 1.0f / float(VOXEL_GRID_SIZE);
const vec2 HALF_VOXEL_SIZE = vec2(VOXEL_GRID_SIZE_INV / 2.0f);

layout(binding = 0, set = VOXEL_SET) uniform WorldToVoxelBuffer
{
	mat4 WorldToVoxel;
	mat4 WorldToVoxelInv;
};

layout(binding = 1, set = VOXEL_SET, rgba8) uniform image3D VoxelBaseColor;

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

#if DEBUG_VOXELS

layout(binding = 2, set = VOXEL_SET, std430) buffer VoxelPositionBuffer
{
	int VoxelPositions[];
};

layout(binding = 3, set = VOXEL_SET, std430) buffer VoxelDrawIndirectBuffer
{
	DrawIndirectCommand VoxelDrawIndirect;
};

int EncodeVoxelPosition(ivec3 VoxelPosition)
{
	return (VoxelPosition.z << 20) | (VoxelPosition.y << 10) | (VoxelPosition.x << 0);
}

ivec3 DecodeVoxelPosition(int VoxelPosition)
{
	const int Mask = 0x000003FF;
	return ivec3((VoxelPosition >> 0)& Mask, (VoxelPosition >> 10)& Mask, (VoxelPosition >> 20)& Mask);
}

#endif

#endif
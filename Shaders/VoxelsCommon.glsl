#ifndef VOXELS_COMMON
#define VOXELS_COMMON
#include "Common.glsl"

layout(binding = 0, set = VOXEL_SET) uniform WorldToVoxelBuffer
{
	mat4 WorldToVoxel;
	mat4 WorldToVoxelInv;
};

layout(binding = 1, set = VOXEL_SET, rgba8) uniform image3D VoxelBaseColor;

layout(binding = 2, set = VOXEL_SET, rgba16f) uniform image3D VoxelNormal;

layout(binding = 3, set = VOXEL_SET, rgba8) uniform image3D VoxelRadiance;

const float VOXEL_SIZE = 1.0f / (float(VOXEL_GRID_SIZE) * float(VOXEL_SCALE));
const vec2 HALF_VOXEL_SIZE = vec2(VOXEL_SIZE / 2.0f);

vec3 TransformWorldToVoxel(in vec3 WorldPosition)
{
	vec4 VoxelPosition = WorldToVoxel * vec4(WorldPosition.xyz, 1);
	return VoxelPosition.xyz;
}

vec3 TransformWorldToVoxelUVW(in vec3 WorldPosition)
{
	vec3 VoxelUVW = TransformWorldToVoxel(WorldPosition);
	VoxelUVW.xy = VoxelUVW.xy / 2.0 + 0.5;
	return VoxelUVW;
}

ivec3 TransformWorldToVoxelGridCoord(in vec3 WorldPosition)
{
	vec3 VoxelUVW = TransformWorldToVoxelUVW(WorldPosition);
	VoxelUVW *= VOXEL_GRID_SIZE;
	return ivec3(VoxelUVW);
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

layout(binding = 4, set = VOXEL_SET, std430) buffer VoxelPositionBuffer
{
	int VoxelPositions[];
};

layout(binding = 5, set = VOXEL_SET, std430) buffer VoxelDrawIndirectBuffer
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
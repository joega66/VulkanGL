#ifndef VOXELS_COMMON
#define VOXELS_COMMON
#include "Common.glsl"

#ifdef VOXEL_SET
layout(binding = 0, set = VOXEL_SET) uniform WorldToVoxelBuffer
{
	mat4 _WorldToVoxel;
	mat4 _WorldToVoxelInv;
	vec4 _VoxelSize;
};
#endif

vec3 TransformWorldToVoxel(in vec3 WorldPosition)
{
	vec4 VoxelPosition = _WorldToVoxel * vec4(WorldPosition.xyz, 1);
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

#if defined(DEBUG_VOXELS) && defined(VOXEL_SET)

layout(binding = 3, set = VOXEL_SET, std430) buffer VoxelPositionBuffer
{
	int VoxelPositions[];
};

layout(binding = 4, set = VOXEL_SET, std430) buffer VoxelDrawIndirectBuffer
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
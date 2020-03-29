#include "Common.glsl"
#include "SceneCommon.glsl"
#define VOXEL_SET 1
#include "VoxelsCommon.glsl"

#ifndef VOXEL_GRID_SIZE
#define VOXEL_GRID_SIZE 512
#endif

#ifdef VERTEX_SHADER

layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec4 OutVoxelColor;
layout(location = 2) out int OutVoxelGridZ;

void main()
{
	ivec3 VoxelPosition = DecodeVoxelPosition(VoxelPositions[gl_VertexIndex]);

	OutPosition = TransformVoxelToWorld(VoxelPosition);
	OutVoxelColor = imageLoad(VoxelRadiance, VoxelPosition);
	OutVoxelGridZ = VoxelPosition.z;

	gl_Position = vec4(OutPosition, 1);
}

#endif

#ifdef GEOMETRY_SHADER

layout(points) in;
layout(triangle_strip, max_vertices = 14) out;

layout(location = 0) in vec3 InVoxelPosition[];
layout(location = 1) in vec4 InVoxelColor[];
layout(location = 2) in int InVoxelGridZ[];

layout(location = 0) out vec4 OutVoxelColor;

void main()
{
	const float CubeZOffset = InVoxelGridZ[0] % 2 == 0 ? 0.0f : float(VOXEL_SCALE) / 2.0f;

	OutVoxelColor = InVoxelColor[0];

	for (uint i = 0; i < 14; i++)
	{
		vec3 CubePosition = CreateCube(i) * float(VOXEL_SCALE);
		CubePosition.z += CubeZOffset;
		vec3 VoxelPosition = InVoxelPosition[0];
		VoxelPosition += vec3(CubePosition.x, -CubePosition.y, -CubePosition.z);
		gl_Position = Camera.WorldToClip * vec4(VoxelPosition.xyz, 1);
		EmitVertex();
	}
	EndPrimitive();
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) in vec4 InVoxelColor;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = vec4(InVoxelColor.rgb, 0.5);
}

#endif
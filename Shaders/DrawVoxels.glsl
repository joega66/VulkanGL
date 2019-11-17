#include "Common.glsl"
#include "SceneCommon.glsl"

#ifndef VOXEL_GRID_SIZE
#define VOXEL_GRID_SIZE 256
#endif

layout(binding = 0, set = 1) uniform WorldToVoxelBuffer
{
	mat4 WorldToVoxel;
	mat4 WorldToVoxelInv;
};

layout(binding = 1, set = 1, rgba8) readonly uniform image3D VoxelDiffuseGI;

layout(binding = 2, set = 1, std430) readonly buffer VoxelPositionBuffer
{
	ivec3 VoxelPositions[];
};

#ifdef VERTEX_SHADER

layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec4 OutVoxelColor;

void main()
{
	OutPosition = VoxelPositions[gl_VertexIndex];
	OutPosition /= VOXEL_GRID_SIZE;
	OutPosition.xy = (OutPosition.xy - 0.5) * 2;
	OutPosition *= 5;
	OutPosition = vec3(WorldToVoxelInv * vec4(OutPosition, 1));
	OutPosition += vec3(-700, 500, 750);

	OutVoxelColor = imageLoad(VoxelDiffuseGI, VoxelPositions[gl_VertexIndex]);

	gl_Position = vec4(OutPosition, 1);
}

#endif

#ifdef GEOMETRY_SHADER

layout(points) in;
layout(triangle_strip, max_vertices = 14) out;

layout(location = 0) in vec3 InVoxelPosition[];
layout(location = 1) in vec4 InVoxelColor[];

layout(location = 0) out vec4 OutVoxelColor;

const float CubeScale = 5;

void main()
{
	if (OutVoxelColor.a >= 0.0f)
	{
		for (uint i = 0; i < 14; i++)
		{
			vec3 Pos = InVoxelPosition[0];
			Pos += CreateCube(i) * CubeScale;
			OutVoxelColor = InVoxelColor[0];
			gl_Position = View.WorldToClip * vec4(Pos.xyz, 1);
			EmitVertex();
		}
		EndPrimitive();
	}
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) in vec4 InVoxelColor;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = vec4(InVoxelColor.xyz, 1);
}

#endif
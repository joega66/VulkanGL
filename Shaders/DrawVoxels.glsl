#include "Common.glsl"
#include "SceneCommon.glsl"

#ifndef VOXEL_GRID_SIZE
#define VOXEL_GRID_SIZE 256
#endif

layout(binding = 0, set = 1) uniform VoxelOrthoProjBuffer
{
	mat4 Transform;
} VoxelOrthoProj;

//layout(binding = 1, set = 1, rgba8) readonly uniform image3D VoxelColor;

layout(binding = 1, set = 2, std430) readonly buffer VoxelColorBuffer
{
	uint NumVoxels;
	uint Data[];
} VoxelColors;

layout(binding = 2, set = 2, std430) readonly buffer VoxelPositionBuffer
{
	vec3 Data[];
} VoxelPositions;

#ifdef VERTEX_SHADER

layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec4 OutVoxelColor;

void main()
{
	//uvec3 VoxelTexCoord = Unflatten3D(gl_VertexIndex, uvec3(VOXEL_GRID_SIZE));
	//OutVoxelColor = imageLoad(VoxelColor, ivec3(VoxelTexCoord));
	OutPosition = VoxelPositions.Data[gl_VertexIndex];
	OutVoxelColor = IntToColor(VoxelColors.Data[gl_VertexIndex]);
	gl_Position = vec4(OutPosition, 1);
}

#endif

#ifdef GEOMETRY_SHADER

layout(points) in;
layout(triangle_strip, max_vertices = 14) out;

layout(location = 0) in vec3 InVoxelPosition[];
layout(location = 1) in vec4 InVoxelColor[];

layout(location = 0) out vec4 OutVoxelColor;

void main()
{
	if (InVoxelColor[0].a > 0)
	{
		for (uint i = 0; i < 14; i++)
		{
			vec3 Pos = InVoxelPosition[0];
			Pos += (CreateCube(i) - vec3(0, 1, 0)) * 2;
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
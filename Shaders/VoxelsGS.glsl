layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

#include "MeshCommon.glsl"
#include "SceneCommon.glsl"
#define VOXEL_SET 3
#include "VoxelsCommon.glsl"

layout(location = 4) out vec4 OutTriangleAABB;

void main()
{
	/* 1. Find the axis that maximizes the projected area of this triangle. */
	const vec3 FaceNormal = abs(InNormal[0] + InNormal[1] + InNormal[2]);
	uint Axis = FaceNormal.y > FaceNormal.x ? 1 : 0;
	Axis = FaceNormal.z > FaceNormal[Axis] ? 2 : Axis;

	vec3 ClipSpacePositions[3];
	vec4 TriangleAABB = vec4(1.0f, 1.0f, -1.0f, -1.0f);

	for (uint i = 0; i < 3; i++)
	{
		ClipSpacePositions[i] = TransformWorldToVoxel(InPosition[i].xyz);
		
		if (Axis == 0)
		{
			ClipSpacePositions[i].xyz = ClipSpacePositions[i].zyx;
		}
		else if (Axis == 1)
		{
			ClipSpacePositions[i].xyz = ClipSpacePositions[i].xzy;
		}

		TriangleAABB.xy = min(TriangleAABB.xy, ClipSpacePositions[i].xy);
		TriangleAABB.zw = max(TriangleAABB.zw, ClipSpacePositions[i].xy);
	};

	TriangleAABB.xy -= HALF_VOXEL_SIZE;
	TriangleAABB.zw += HALF_VOXEL_SIZE;

	//OutTriangleAABB = TriangleAABB;
	OutTriangleAABB = vec4(-1.0f, -1.0f, 1.0f, 1.0f);

	/* 2. Conservative rasterization (GPU Gems 2: Chapter 42) */

	// Find the input triangle edges.
	vec3 Edges[3] = { vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0) };
	Edges[0].xy = ClipSpacePositions[1].xy - ClipSpacePositions[0].xy;
	Edges[1].xy = ClipSpacePositions[2].xy - ClipSpacePositions[1].xy;
	Edges[2].xy = ClipSpacePositions[0].xy - ClipSpacePositions[2].xy;

	// Find the edge normals.
	vec2 Normals[3];
	Normals[0] = CalcLineNormal(Edges[0].xy);
	Normals[1] = CalcLineNormal(Edges[1].xy);
	Normals[2] = CalcLineNormal(Edges[2].xy);

	// Move the edges by the worst-case semidiagonal.
	Edges[0].xy += dot(HALF_VOXEL_SIZE, Normals[0]);
	Edges[1].xy += dot(HALF_VOXEL_SIZE, Normals[1]);
	Edges[2].xy += dot(HALF_VOXEL_SIZE, Normals[2]);

	// Find the bounding triangle.
	ClipSpacePositions[0].xy = cross(Edges[0], Edges[1]).xy;
	ClipSpacePositions[1].xy = cross(Edges[1], Edges[2]).xy;
	ClipSpacePositions[2].xy = cross(Edges[0], Edges[2]).xy;

	for (uint i = 0; i < 3; i++)
	{
		gl_Position.xyz = ClipSpacePositions[i];
		gl_Position.w = 1.0;
		SetGSInterpolants(i);
		EmitVertex();
	}

	EndPrimitive();
}
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

#include "MeshCommon.glsl"
#include "SceneCommon.glsl"
#define VOXEL_SET 2
#include "VoxelsCommon.glsl"

void main()
{
	// Find the axis that maximizes the projected area of this triangle.
	const vec3 FaceNormal = abs(InNormal[0] + InNormal[1] + InNormal[2]);
	uint Axis = FaceNormal.y > FaceNormal.x ? 1 : 0;
	Axis = FaceNormal.z > FaceNormal[Axis] ? 2 : Axis;

	vec3 Positions[3];
	
	for (uint i = 0; i < 3; i++)
	{
		Positions[i] = TransformWorldToVoxel(InPosition[i].xyz);

		if (Axis == 0)
		{
			Positions[i].xyz = Positions[i].zyx;
		}
		else if (Axis == 1)
		{
			Positions[i].xyz = Positions[i].xzy;
		}
	};

	for (uint i = 0; i < 3; i++)
	{
		gl_Position.xyz = Positions[i];
		gl_Position.w = 1;
		SetGSInterpolants(i);
		EmitVertex();
	}

	EndPrimitive();
}
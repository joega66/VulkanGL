layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

#include "MeshCommon.glsl"
#include "SceneCommon.glsl"
#include "VoxelsCommon.glsl"

const vec3 VoxelProbeCenter = vec3(-700, 500, 750);
const float VoxelSize = 4;

void main()
{
	// Find the axis that maximizes the projected area of this triangle.
	const vec3 FaceNormal = abs(InNormal[0] + InNormal[1] + InNormal[2]);
	uint Axis = FaceNormal.y > FaceNormal.x ? 1 : 0;
	Axis = FaceNormal.z > FaceNormal[Axis] ? 2 : Axis;

	vec4 Positions[3];

	for (uint i = 0; i < 3; i++)
	{
		Positions[i] = VoxelOrthoProj * vec4(InPosition[i].xyz - vec3(VoxelProbeCenter), 1);

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
		gl_Position = Positions[i];
		gl_Position.w += VoxelSize;
		SetGSInterpolants(i);
		EmitVertex();
	}

	EndPrimitive();
}
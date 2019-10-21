layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

#include "MeshCommon.glsl"
#include "SceneCommon.glsl"
#include "VoxelsCommon.glsl"

void main()
{
	// Find the axis that maximizes the projected area of this triangle.
	const vec3 FaceNormal = abs(InNormal[0] + InNormal[1] + InNormal[2]);
	uint Axis = FaceNormal[1] > FaceNormal[0] ? 1 : 0;
	Axis = FaceNormal[2] > FaceNormal[Axis] ? 2 : Axis;

	for (uint i = 0; i < 3; i++)
	{
		gl_Position = VoxelOrthoProj.Transform * InPosition[i];
		SetGSInterpolants(i);
		EmitVertex();
	}

	EndPrimitive();
}
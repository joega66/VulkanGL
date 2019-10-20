layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

#include "VoxelsCommon.glsl"
#ifdef STATIC_MESH
#include "StaticMeshVS.glsl"
#endif

layout(location = 4) flat out int OutAxis;

void main()
{
	const vec3 FaceNormal = normalize(cross(Position[1] - Position[0], Position[2] - Position[0]));
	const float NdotXAxis = abs(FaceNormal.x);
	const float NdotYAxis = abs(FaceNormal.y);
	const float NdotZAxis = abs(FaceNormal.z);

	mat4 OrthoProj;

	// Find the axis that maximizes the projected area of this triangle.
	if (NdotXAxis > NdotYAxis && NdotXAxis > NdotZAxis)
	{
		OrthoProj = VoxelOrthoProj.X;
		OutAxis = 1;
	}
	else if (NdotYAxis > NdotXAxis && NdotYAxis > NdotZAxis)
	{
		OrthoProj = VoxelOrthoProj.Y;
		OutAxis = 2;
	}
	else
	{
		OrthoProj = VoxelOrthoProj.Z;
		OutAxis = 3;
	}

	gl_Position = OrthoProj * vec4(Position[0], 1.0);
	SetGSInterpolants(0);
	EmitVertex();

	gl_Position = OrthoProj * vec4(Position[1], 1.0);
	SetGSInterpolants(1);
	EmitVertex();

	gl_Position = OrthoProj * vec4(Position[2], 1.0);
	SetGSInterpolants(2);
	EmitVertex();

	EndPrimitive();
}
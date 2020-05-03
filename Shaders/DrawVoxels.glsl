#include "Common.glsl"
#include "CameraCommon.glsl"
#define VOXEL_SET 1
#include "VoxelsCommon.glsl"

#ifdef VERTEX_SHADER

layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec4 OutVoxelColor;
layout(location = 2) out int OutVoxelGridZ;

layout(constant_id = 0) const uint VoxelDebugMode = 1;

void main()
{
	ivec3 VoxelPosition = DecodeVoxelPosition(VoxelPositions[gl_VertexIndex]);

	OutPosition = TransformVoxelToWorld(VoxelPosition);

	// Must match Voxels.h
	if (VoxelDebugMode == 1)
	{
		OutVoxelColor = imageLoad(VoxelRadiance, VoxelPosition);
	}
	else if (VoxelDebugMode == 2)
	{
		OutVoxelColor = imageLoad(VoxelBaseColor, VoxelPosition);
	}
	else // 3
	{
		OutVoxelColor = imageLoad(VoxelNormal, VoxelPosition);
	}
	
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
	const float CubeZOffset = InVoxelGridZ[0] % 2 == 0 ? 0.0f : _VoxelSize.z / 2.0f;

	OutVoxelColor = InVoxelColor[0];

	for (uint i = 0; i < 14; i++)
	{
		vec3 CubePosition = CreateCube(i) * _VoxelSize.z;
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
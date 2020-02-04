#include "Common.glsl"
#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "MaterialCommon.glsl"
#include "LightingCommon.glsl"
#define VOXEL_SET 4
#include "VoxelsCommon.glsl"

layout(location = 4) in vec4 TriangleAABB;

void main()
{
	vec2 NDCPosition = vec2(gl_FragCoord.xy / vec2(VOXEL_GRID_SIZE)) * 2.0f - 1.0f;

	if (!( NDCPosition.x >= TriangleAABB.x && NDCPosition.x <= TriangleAABB.z 
		&& NDCPosition.y >= TriangleAABB.y && NDCPosition.y <= TriangleAABB.w ))
	{
		discard;
	}

	SurfaceData Surface = Surface_Get();

	Material_DiscardMaskedPixel(Surface);

	MaterialData Material = Material_Get(Surface);

	vec4 Color = Shade(Surface, Material);

	vec3 VoxelTexCoord = TransformWorldToVoxel(Surface.WorldPosition);
	VoxelTexCoord.xy = VoxelTexCoord.xy / 2.0 + 0.5;
	VoxelTexCoord *= VOXEL_GRID_SIZE;

	imageStore(VoxelDiffuseGI, ivec3(VoxelTexCoord), vec4(Color));

	uint VoxelIndex = atomicAdd(VoxelDrawIndirect.VertexCount, 1);
	VoxelPositions[VoxelIndex] = EncodeVoxelPosition(ivec3(VoxelTexCoord));
}
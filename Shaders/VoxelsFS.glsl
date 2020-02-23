#include "Common.glsl"
#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "MaterialCommon.glsl"
#include "LightingCommon.glsl"
#define VOXEL_SET 3
#include "VoxelsCommon.glsl"

void main()
{
	vec2 NDCPosition = vec2(gl_FragCoord.xy / vec2(VOXEL_GRID_SIZE)) * 2.0f - 1.0f;

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
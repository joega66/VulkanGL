#include "Common.glsl"
#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "LightingCommon.glsl"
#define VOXEL_SET 2
#include "VoxelsCommon.glsl"

void main()
{
	MaterialParams Material = GetMaterial();

	vec4 Color = Shade(Material);

	vec3 VoxelTexCoord = TransformWorldToVoxel(Material.Position);
	VoxelTexCoord.xy = VoxelTexCoord.xy / 2 + 0.5;
	VoxelTexCoord *= VOXEL_GRID_SIZE;

	imageStore(VoxelDiffuseGI, ivec3(VoxelTexCoord), vec4(Color));

	uint VoxelIndex = atomicAdd(VoxelDrawIndirect.VertexCount, 1);
	VoxelPositions[VoxelIndex] = ivec3(VoxelTexCoord);
}
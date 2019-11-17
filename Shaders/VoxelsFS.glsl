#include "Common.glsl"
#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "LightingCommon.glsl"
#include "VoxelsCommon.glsl"

void main()
{
	MaterialParams Material = GetMaterial();

	vec4 Color = Shade(Material);
	//vec4 Color = vec4(Material.Albedo, 1);

	vec3 VoxelTexCoord = vec3(WorldToVoxel * vec4(Material.Position - vec3(-700, 500, 750), 1));
	VoxelTexCoord /= 5;
	VoxelTexCoord.xy = VoxelTexCoord.xy / 2 + 0.5;
	VoxelTexCoord *= VOXEL_GRID_SIZE;

	imageStore(VoxelDiffuseGI, ivec3(VoxelTexCoord), vec4(Color));

	uint VoxelIndex = atomicAdd(VoxelDrawIndirect.VertexCount, 1);
	VoxelPositions[VoxelIndex] = ivec3(VoxelTexCoord);
}
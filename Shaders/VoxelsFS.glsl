#include "Common.glsl"
#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "LightingCommon.glsl"
#include "VoxelsCommon.glsl"

void main()
{
	MaterialParams Material = GetMaterial();

	vec4 Color = Shade(Material);

	ivec3 VoxelTexCoord = ivec3(gl_FragCoord.x, gl_FragCoord.y, gl_FragCoord.z);

	//imageStore(VoxelImage3d, VoxelTexCoord, vec4(Color.xyz, 1.0));
	imageStore(VoxelImage3d, VoxelTexCoord, vec4(Material.Albedo, 1.0));
}
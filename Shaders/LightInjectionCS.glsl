#include "SceneCommon.glsl"
#define VOXEL_SET 1
#include "VoxelsCommon.glsl"
#include "MaterialInterface.glsl"
#include "MeshCommon.glsl"
#include "LightingCommon.glsl"

#define LIGHT_SET 2
layout(binding = 0, set = LIGHT_SET, r32f) uniform readonly image2D ShadowMap;
layout(binding = 1, set = LIGHT_SET) uniform LightViewProjBuffer
{
	mat4 LightViewProj;
	mat4 InvLightViewProj;
};

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	ivec2 ShadowMapSize = imageSize(ShadowMap);
	if (gl_GlobalInvocationID.x >= ShadowMapSize.x * ShadowMapSize.y)
		return;

	// @todo Reproject(InvMatrix, UV, Depth)...

	// 1. Project light depth into the volume.
	float LightDepth = imageLoad(ShadowMap, ivec2(gl_GlobalInvocationID.xy)).r;
	vec2 ClipSpace = vec2(gl_GlobalInvocationID.xy) / ShadowMapSize;
	ClipSpace = (ClipSpace - 0.5f) * 2.0f;
	vec4 ClipSpaceH = vec4(ClipSpace, LightDepth, 1.0f);
	vec4 WorldPosition = InvLightViewProj * ClipSpaceH;
	WorldPosition.xyz /= WorldPosition.w;

	ivec3 VoxelGridCoord = TransformWorldToVoxelGridCoord(WorldPosition.xyz);

	// 2. Load the material from the voxel grid.
	vec4 BaseColor = imageLoad(VoxelBaseColor, VoxelGridCoord);

	MaterialData Material;
	Material.BaseColor = BaseColor.rgb;
	Material.Alpha = BaseColor.a;
	Material.Roughness = 0.0f;
	Material.Metallicity = 0.0f;

	SurfaceData Surface;
	Surface.WorldPosition = WorldPosition.xyz;
	Surface.WorldNormal = imageLoad(VoxelNormal, VoxelGridCoord).rgb;

	// 3. Compute lighting.
	vec3 DirectLighting = Shade(Surface, Material).rgb;

	// 4. Inject into the volume.
	imageStore(VoxelRadiance, VoxelGridCoord, vec4(DirectLighting, 1.0f));
}
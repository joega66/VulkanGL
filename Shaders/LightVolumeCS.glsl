#include "CameraCommon.glsl"
#define VOXEL_SET 1
#include "VoxelsCommon.glsl"
#include "LightingCommon.glsl"

#define LIGHT_SET 2
layout(binding = 0, set = LIGHT_SET) uniform LightViewProjUniform
{
	mat4 LightViewProj;
	mat4 InvLightViewProj;
};
layout(binding = 1, set = LIGHT_SET) uniform VolumeLightingUniform
{
	ivec4 ShadowMapSize;
	vec4 L;
	vec4 Radiance;
};

#define TEXTURE_SET 3
#include "SceneResources.glsl"

layout(push_constant) uniform PushConstants
{
	uint ShadowMap;
};

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, ShadowMapSize.xy)))
		return;

	// 1. Project light depth into the volume.
	float LightDepth = Load(ShadowMap, ivec2(gl_GlobalInvocationID.xy)).r;
	vec2 ClipSpace = vec2(gl_GlobalInvocationID.xy) / ShadowMapSize.xy;
	ClipSpace = (ClipSpace - 0.5f) * 2.0f;
	vec4 ClipSpaceH = vec4(ClipSpace, LightDepth, 1.0f);
	vec4 WorldPosition = InvLightViewProj * ClipSpaceH;
	WorldPosition.xyz /= WorldPosition.w;

	ivec3 VoxelGridCoord = TransformWorldToVoxelGridCoord(WorldPosition.xyz);

	// 2. Load the material from the voxel grid.
	vec4 BaseColor = imageLoad(VoxelBaseColor, VoxelGridCoord);

	// @todo Metallic, Roughness
	MaterialData Material;
	Material.BaseColor = BaseColor.rgb;
	Material.Metallic = 0.0f;
	Material.Roughness = 0.0f;
	Material.SpecularColor = mix(vec3(0.04), Material.BaseColor, Material.Metallic);
	Material.DiffuseColor = Diffuse_BRDF(Material.BaseColor);

	SurfaceData Surface;
	Surface.WorldPosition = WorldPosition.xyz;
	Surface.WorldNormal = imageLoad(VoxelNormal, VoxelGridCoord).rgb;
	
	LightParams Light;
	Light.L = L.xyz;
	Light.Radiance = Radiance.rgb;

	if (dot(Surface.WorldNormal, Light.L) < 0.0)
	{
		// The voxel field is too sparse to capture thin geometry, 
		// so flip the normal if it's facing away from the light.
		Surface.WorldNormal *= -1;
	}

	// 3. Compute lighting.
	vec3 V = normalize(Camera.Position - Surface.WorldPosition);

	vec3 Lo = DirectLighting(V, Light, Surface, Material).rgb;

	imageStore(VoxelRadiance, VoxelGridCoord, vec4(Lo, 1.0f));
}
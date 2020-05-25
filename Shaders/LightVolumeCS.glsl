#include "CameraCommon.glsl"
#include "VoxelsCommon.glsl"
#include "LightingCommon.glsl"

#define LIGHT_SET 1
layout(binding = 0, set = LIGHT_SET) uniform LightViewProjUniform
{
	mat4 _LightViewProj;
	mat4 _LightViewProjInv;
};
layout(binding = 1, set = LIGHT_SET) uniform VolumeLightingUniform
{
	ivec4 _ShadowMapSize;
	vec4 _L;
	vec4 _Radiance;
};

#define TEXTURE_SET 2
#define TEXTURE_3D_SET 3
#define IMAGE_3D_SET 4
#include "SceneResources.glsl"

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, _ShadowMapSize.xy)))
		return;

	// 1. Project light depth into the volume.
	float LightDepth = Load(_ShadowMap, ivec2(gl_GlobalInvocationID.xy)).r;
	vec2 ClipSpace = vec2(gl_GlobalInvocationID.xy) / _ShadowMapSize.xy;
	ClipSpace = (ClipSpace - 0.5f) * 2.0f;
	vec4 ClipSpaceH = vec4(ClipSpace, LightDepth, 1.0f);
	vec4 WorldPosition = _LightViewProjInv * ClipSpaceH;
	WorldPosition.xyz /= WorldPosition.w;

	ivec3 VoxelGridCoord = TransformWorldToVoxelGridCoord(WorldPosition.xyz);

	// 2. Load the material from the voxel grid.
	vec4 BaseColor = TexelFetch(_VoxelBaseColor, VoxelGridCoord, 0);

	// @todo Metallic, Roughness
	MaterialData Material;
	Material.BaseColor = BaseColor.rgb;
	Material.Metallic = 0.0f;
	Material.Roughness = 0.0f;
	Material.SpecularColor = mix(vec3(0.04), Material.BaseColor, Material.Metallic);
	Material.DiffuseColor = Diffuse_BRDF(Material.BaseColor);

	SurfaceData Surface;
	Surface.WorldPosition = WorldPosition.xyz;
	Surface.WorldNormal = TexelFetch(_VoxelNormal, VoxelGridCoord, 0).rgb;
	
	LightData Light;
	Light.L = _L.xyz;
	Light.Radiance = _Radiance.rgb;

	if (dot(Surface.WorldNormal, Light.L) < 0.0)
	{
		// The voxel field is too sparse to capture thin geometry, 
		// so flip the normal if it's facing away from the light.
		Surface.WorldNormal *= -1;
	}

	// 3. Compute lighting.
	vec3 V = normalize(Camera.Position - Surface.WorldPosition);

	vec3 Ld = DirectLighting(V, Light, Surface, Material).rgb;

	ImageStore(_VoxelRadiance, VoxelGridCoord, vec4(Ld, 1.0f));
}
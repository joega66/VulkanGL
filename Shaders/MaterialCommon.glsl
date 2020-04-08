#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON
#include "MaterialInterface.glsl"
#define SCENE_TEXTURES_SET 2
#include "SceneResources.glsl"

layout(push_constant) uniform MaterialConstants
{
	uint BaseColorIndex;
	uint MetallicRoughnessIndex;
	uint NormalIndex;
	float Metallic;
	float Roughness;
} _Material;

layout(constant_id = 0) const bool HasMetallicRoughnessTexture = false;
layout(constant_id = 1) const bool IsMasked = false;
layout(constant_id = 2) const bool HasNormalTexture = false;

MaterialData Material_Get(SurfaceData Surface)
{
	vec4 BaseColor = texture(SceneTextures[nonuniformEXT(_Material.BaseColorIndex)], Surface.UV).rgba;
	
	MaterialData Material;
	Material.BaseColor = BaseColor.rgb;

	if (HasMetallicRoughnessTexture)
	{
		vec2 MetallicRoughness = texture(SceneTextures[nonuniformEXT(_Material.MetallicRoughnessIndex)], Surface.UV).gb;
		Material.Metallic = MetallicRoughness.x;
		Material.Roughness = MetallicRoughness.y;
	}
	else
	{
		Material.Metallic = _Material.Metallic;
		Material.Roughness = _Material.Roughness;
	}

	if (IsMasked)
	{
		Material.Alpha = BaseColor.a;
	}
	else
	{
		Material.Alpha = 1.0f;
	}

	Material.SpecularColor = mix(vec3(0.04), Material.BaseColor, Material.Metallic);

	return Material;
}

#ifdef FRAGMENT_SHADER

void Material_DiscardMaskedPixel(SurfaceData Surface)
{
	float Alpha = texture(SceneTextures[nonuniformEXT(_Material.BaseColorIndex)], Surface.UV).a;
	if (Alpha <= 0)
	{
		discard;
	}
}

/** Reference: http://www.thetenthplanet.de/archives/1180 */
mat3 CotangentFrame(vec3 WorldNormal, vec3 WorldPosition, vec2 UV)
{
	vec3 DP1 = dFdx(WorldPosition);
	vec3 DP2 = dFdy(WorldPosition);
	vec2 DUV1 = dFdx(UV);
	vec2 DUV2 = dFdy(UV);

	vec3 DP2Perp = cross(DP2, WorldNormal);
	vec3 DP1Perp = cross(WorldNormal, DP1);
	vec3 T = DP2Perp * DUV1.x + DP1Perp * DUV2.x;
	vec3 B = DP2Perp * DUV1.y + DP1Perp * DUV2.y;

	float InvMax = inversesqrt(max(dot(T, T), dot(B, B)));
	return mat3(T * InvMax, B * InvMax, WorldNormal);
}

void Material_NormalMapping(inout SurfaceData Surface, vec3 V)
{
	if (HasNormalTexture)
	{
		const vec3 MapNormal = texture(SceneTextures[nonuniformEXT(_Material.NormalIndex)], Surface.UV).xyz * 2.0 - 1.0;
		Surface.WorldNormal = normalize(CotangentFrame(Surface.WorldNormal, V, Surface.UV) * MapNormal);
	}
}

#endif

#endif
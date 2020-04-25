#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON
#include "MaterialInterface.glsl"
#include "SceneResources.glsl"

layout(push_constant) uniform MaterialConstants
{
	uint BaseColor;
	uint MetallicRoughness;
	uint Normal;
	uint Emissive;
	uint Sampler;
	float Metallic;
	float Roughness;
	vec3 EmissiveFactor;
} _Material;

layout(constant_id = 0) const bool HasMetallicRoughnessTexture = false;
layout(constant_id = 1) const bool IsMasked = false;
layout(constant_id = 2) const bool HasNormalTexture = false;
layout(constant_id = 3) const bool HasEmissiveTexture = false;

MaterialData Material_Get(SurfaceData Surface)
{
	vec4 BaseColor = Sample2D(_Material.BaseColor, _Material.Sampler, Surface.UV);
	
	MaterialData Material;
	Material.BaseColor = BaseColor.rgb;

	if (HasMetallicRoughnessTexture)
	{
		vec2 MetallicRoughness = Sample2D(_Material.MetallicRoughness, _Material.Sampler, Surface.UV).gb;
		Material.Metallic = MetallicRoughness.x;
		Material.Roughness = MetallicRoughness.y;
	}
	else
	{
		Material.Metallic = _Material.Metallic;
		Material.Roughness = _Material.Roughness;
	}

	return Material;
}

#ifdef FRAGMENT_SHADER

void Material_DiscardMaskedPixel(SurfaceData Surface)
{
	if (IsMasked)
	{
		float Alpha = Sample2D(_Material.BaseColor, _Material.Sampler, Surface.UV).a;
		if (Alpha <= 0)
		{
			discard;
		}
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
		const vec3 MapNormal = Sample2D(_Material.Normal, _Material.Sampler, Surface.UV).xyz * 2.0 - 1.0;
		Surface.WorldNormal = normalize(CotangentFrame(Surface.WorldNormal, V, Surface.UV) * MapNormal);
	}
}

void Material_Emissive(SurfaceData Surface, inout vec3 Color)
{
	if (HasEmissiveTexture)
	{
		vec3 EmissiveColor = Sample2D(_Material.Emissive, _Material.Sampler, Surface.UV).rgb;
		EmissiveColor *= _Material.EmissiveFactor;
		Color += EmissiveColor;
	}
}

#endif

#endif
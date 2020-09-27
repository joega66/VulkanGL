#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON
#include "MaterialInterface.glsl"
#include "SceneResources.glsl"

layout(push_constant) uniform MaterialConstants
{
	layout(offset = 4) uint baseColor;
	uint metallicRoughness;
	uint normal;
	uint emissive;
	uint samplerID;
	float metallic;
	float roughness;
	vec3 emissiveFactor;
} _Material;

layout(constant_id = 0) const bool _HasMetallicRoughnessTexture = false;
layout(constant_id = 1) const bool _IsMasked = false;
layout(constant_id = 2) const bool _HasNormalTexture = false;
layout(constant_id = 3) const bool _HasEmissiveTexture = false;

MaterialData Material_Get(SurfaceData surface)
{
	vec4 baseColor = Sample2D(_Material.baseColor, _Material.samplerID, surface.uv);
	
	MaterialData material;
	material.baseColor = baseColor.rgb;

	if (_HasMetallicRoughnessTexture)
	{
		vec2 metallicRoughness = Sample2D(_Material.metallicRoughness, _Material.samplerID, surface.uv).gb;
		material.metallic = metallicRoughness.y;
		material.roughness = 1.0 - metallicRoughness.x;
	}
	else
	{
		material.metallic = _Material.metallic;
		material.roughness = _Material.roughness;
	}

	return material;
}

#ifdef FRAGMENT_SHADER

void Material_DiscardMaskedPixel(SurfaceData surface)
{
	if (_IsMasked)
	{
		float alpha = Sample2D(_Material.baseColor, _Material.samplerID, surface.uv).a;
		if (alpha <= 0)
		{
			discard;
		}
	}
}

/** Reference: http://www.thetenthplanet.de/archives/1180 */
mat3 CotangentFrame(vec3 worldNormal, vec3 worldPosition, vec2 uv)
{
	vec3 dp1 = dFdx(worldPosition);
	vec3 dp2 = dFdy(worldPosition);
	vec2 duv1 = dFdx(uv);
	vec2 duv2 = dFdy(uv);

	vec3 dp2Perp = cross(dp2, worldNormal);
	vec3 dp1Perp = cross(worldNormal, dp1);
	vec3 t = dp2Perp * duv1.x + dp1Perp * duv2.x;
	vec3 b = dp2Perp * duv1.y + dp1Perp * duv2.y;

	float invMax = inversesqrt(max(dot(t, t), dot(b, b)));
	return mat3(t * invMax, b * invMax, worldNormal);
}

void Material_NormalMapping(inout SurfaceData surface, vec3 v)
{
	if (_HasNormalTexture)
	{
		const vec3 mapNormal = Sample2D(_Material.normal, _Material.samplerID, surface.uv).xyz * 2.0 - 1.0;
		surface.worldNormal = normalize(CotangentFrame(surface.worldNormal, v, surface.uv) * mapNormal);
	}
}

void Material_Emissive(SurfaceData surface, inout vec3 color)
{
	if (_HasEmissiveTexture)
	{
		vec3 emissiveColor = Sample2D(_Material.emissive, _Material.samplerID, surface.uv).rgb;
		emissiveColor *= _Material.emissiveFactor;
		color += emissiveColor;
	}
}

#endif

#endif
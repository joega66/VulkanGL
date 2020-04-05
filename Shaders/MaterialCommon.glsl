#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON
#include "MaterialInterface.glsl"
#define MATERIAL_SET 2

#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 0, set = MATERIAL_SET) uniform sampler2D SceneTextures[];

layout(push_constant) uniform MaterialConstants
{
	uint BaseColorIndex;
	uint MetallicRoughnessIndex;
	float Roughness;
	float Metallicity;
} _Material;

layout(constant_id = 0) const bool HasMetallicRoughnessTexture = false;
layout(constant_id = 1) const bool IsMasked = false;

MaterialData Material_Get(in SurfaceData Surface)
{
	vec4 BaseColor = texture(SceneTextures[nonuniformEXT(_Material.BaseColorIndex)], Surface.UV).rgba;
	
	MaterialData Material;
	Material.BaseColor = BaseColor.rgb;

	if (HasMetallicRoughnessTexture)
	{
		vec2 MetallicRoughness = texture(SceneTextures[nonuniformEXT(_Material.MetallicRoughnessIndex)], Surface.UV).gb;
		Material.Metallicity = MetallicRoughness.x;
		Material.Roughness = MetallicRoughness.y;
	}
	else
	{
		Material.Roughness = _Material.Roughness;
		Material.Metallicity = _Material.Metallicity;
	}

	if (IsMasked)
	{
		Material.Alpha = BaseColor.a;
	}
	else
	{
		Material.Alpha = 1.0f;
	}

	Material.SpecularColor = mix(vec3(0.04), Material.BaseColor, Material.Metallicity);

	return Material;
}

#ifdef FRAGMENT_SHADER
void Material_DiscardMaskedPixel(in SurfaceData Surface)
{
	float Alpha = texture(SceneTextures[nonuniformEXT(_Material.BaseColorIndex)], Surface.UV).a;
	if (Alpha <= 0)
	{
		discard;
	}
}
#endif

#endif
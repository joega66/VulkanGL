#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON
#include "MaterialInterface.glsl"
#define SCENE_TEXTURES_SET 2
#include "SceneResources.glsl"

layout(push_constant) uniform MaterialConstants
{
	uint BaseColorIndex;
	uint MetallicRoughnessIndex;
	float Metallic;
	float Roughness;
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
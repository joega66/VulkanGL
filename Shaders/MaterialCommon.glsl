#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON
#include "MaterialInterface.glsl"
#define MATERIAL_SET 2

layout(binding = 0, set = MATERIAL_SET) uniform sampler2D BaseColorTexture;
layout(binding = 1, set = MATERIAL_SET) uniform sampler2D MetallicRoughnessTexture;
layout(binding = 2, set = MATERIAL_SET) uniform PBRUniformBuffer
{
	float Roughness;
	float Metallicity;
} PBR;

layout(constant_id = 0) const bool HasMetallicRoughnessTexture = false;
layout(constant_id = 1) const bool IsMasked = false;

MaterialData Material_Get(in SurfaceData Surface)
{
	vec4 BaseColor = texture(BaseColorTexture, Surface.UV).rgba;
	
	MaterialData Material;
	Material.BaseColor = BaseColor.rgb;

	if (HasMetallicRoughnessTexture)
	{
		vec2 MetallicRoughness = texture(MetallicRoughnessTexture, Surface.UV).gb;
		Material.Metallicity = MetallicRoughness.x;
		Material.Roughness = MetallicRoughness.y;
	}
	else
	{
		Material.Roughness = PBR.Roughness;
		Material.Metallicity = PBR.Metallicity;
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
	float Alpha = texture(BaseColorTexture, Surface.UV).a;
	if (Alpha <= 0)
	{
		discard;
	}
}
#endif

#endif
#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON

#define MATERIAL_SET 2

layout(binding = 0, set = MATERIAL_SET) uniform sampler2D Diffuse;
layout(binding = 1, set = MATERIAL_SET) uniform sampler2D Specular;
layout(binding = 2, set = MATERIAL_SET) uniform sampler2D Opacity;
layout(binding = 3, set = MATERIAL_SET) uniform sampler2D Bump;
layout(binding = 4, set = MATERIAL_SET) uniform PBRUniformBuffer
{
	float Roughness;
	float Metallicity;
} PBR;

layout(constant_id = 0) const bool HasSpecularMap = false;
layout(constant_id = 1) const bool HasOpacityMap = false;
layout(constant_id = 2) const bool HasBumpMap = false;

struct MaterialData
{
	vec3 Albedo;
	vec3 Specular;
	float Roughness;
	float Metallicity;
	float Alpha;
};

MaterialData Material_Get(in SurfaceData Surface)
{
	MaterialData Material;
	Material.Albedo = texture(Diffuse, Surface.UV).rgb;
	Material.Roughness = PBR.Roughness;
	Material.Metallicity = PBR.Metallicity;

	if (HasOpacityMap)
	{
		Material.Alpha = texture(Opacity, Surface.UV).r;
	}
	else
	{
		Material.Alpha = 1.0f;
	}

	if (HasSpecularMap)
	{
		Material.Specular = texture(Specular, Surface.UV).rgb;
	}
	else
	{
		Material.Specular = vec3(1.0f);
	}

	return Material;
}

#ifdef FRAGMENT_SHADER
void Material_DiscardMaskedPixel(in SurfaceData Surface)
{
	float Alpha = texture(Opacity, Surface.UV).r;
	if (Alpha <= 0)
	{
		discard;
	}
}
#endif

#endif
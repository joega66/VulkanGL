#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON

#define MATERIAL_SET 2

layout(binding = 0, set = MATERIAL_SET) uniform sampler2D Diffuse;
layout(binding = 1, set = MATERIAL_SET) uniform sampler2D Specular;
layout(binding = 2, set = MATERIAL_SET) uniform sampler2D Opacity;
layout(binding = 3, set = MATERIAL_SET) uniform sampler2D Bump;

layout(constant_id = 0) const bool HasSpecularMap = false;
layout(constant_id = 1) const bool HasOpacityMap = false;
layout(constant_id = 2) const bool HasBumpMap = false;

struct MaterialData
{
	vec3 Albedo;
	vec3 Specular;
	float Roughness;
	float Shininess;
	float Alpha;
};

MaterialData Material_Get(in SurfaceData Surface)
{
	MaterialData Material;
	Material.Albedo = texture(Diffuse, Surface.UV).rgb;
	Material.Roughness = 0.25f;
	Material.Shininess = 0.0f;

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
#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON

layout(binding = DIFFUSE_BINDING, set = MATERIAL_SET) uniform sampler2D Diffuse;

#ifdef HAS_OPACITY_MAP
layout(binding = OPACITY_BINDING, set = MATERIAL_SET) uniform sampler2D Opacity;
#endif

struct MaterialParams
{
	vec3 Position;
	vec3 Normal;
	vec3 Albedo;
	float Roughness;
	float Shininess;
	float Alpha;
};

#endif
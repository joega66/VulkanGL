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

struct MaterialParams
{
	vec3 Position;
	vec3 Normal;
	vec3 Albedo;
	vec3 Specular;
	float Roughness;
	float Shininess;
	float Alpha;
};

#endif
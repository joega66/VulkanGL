#ifndef MATERIAL_COMMON
#define MATERIAL_COMMON

layout(binding = LOCAL_TO_WORLD_BINDING, set = MATERIAL_SET) uniform LocalToWorldUniform
{
	mat4 Transform;
} LocalToWorld;

layout(binding = DIFFUSE_BINDING, set = MATERIAL_SET) uniform sampler2D Diffuse;
layout(binding = SPECULAR_BINDING, set = MATERIAL_SET) uniform sampler2D Specular;
layout(binding = OPACITY_BINDING, set = MATERIAL_SET) uniform sampler2D Opacity;
layout(binding = BUMP_BINDING, set = MATERIAL_SET) uniform sampler2D Bump;

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
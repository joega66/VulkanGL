#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec3 InNormal;

#ifdef HAS_DIFFUSE_MAP
uniform sampler2D Diffuse;
#else
uniform DiffuseUniform
{
	vec4 Color;
} Diffuse;
#endif

vec4 GetDiffuse()
{
#ifdef HAS_DIFFUSE_MAP
	return texture(Diffuse, InUV);
#else
	return Diffuse.Color;
#endif
}
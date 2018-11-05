#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec3 InNormal;

layout(binding = 2) uniform sampler2D Diffuse;

vec3 GetDiffuse()
{
	return texture(Diffuse, InUV).rgb;
}
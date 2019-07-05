#include "Common.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec3 InNormal;

uniform sampler2D Diffuse;

vec4 GetDiffuse()
{
	return texture(Diffuse, InUV);
}
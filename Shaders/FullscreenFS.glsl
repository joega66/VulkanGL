#include "Common.glsl"

layout(location = 0) in vec2 InUV;
layout(location = 0) out vec4 OutColor;

#ifndef TEXTURE
#define TEXTURE 1
#endif

layout(binding = 0, set = 0) uniform sampler2D Texture;

void main()
{
#if TEXTURE == 0
	const float zNear = 0.1f;
	const float zFar = 1000.0f;

	float Depth = texture(Texture, InUV).r;
	float LinearDepth = LinearizeDepth(Depth, zNear, zFar) / zFar;
	OutColor = vec4(vec3(LinearDepth), 1.0f);
#elif TEXTURE == 1
	OutColor = texture(Texture, InUV);
#endif
}
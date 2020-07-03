#include "Common.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

#ifndef TEXTURE
#define TEXTURE 1
#endif

layout(binding = 0, set = 0) uniform sampler2D _Texture;

void main()
{
#if TEXTURE == 0
	const float zNear = 0.1f;
	const float zFar = 1000.0f;

	float depth = texture(Texture, inUV).r;
	float linearDepth = LinearizeDepth(depth, zNear, zFar) / zFar;
	outColor = vec4(vec3(linearDepth), 1.0f);
#elif TEXTURE == 1
	outColor = texture(_Texture, inUV);
#endif
}
#include "Common.glsl"

layout(location = 0) in vec2 InUV;
layout(location = 0) out vec4 OutColor;

uniform sampler2D Sampler;

void main()
{
#ifdef DEPTH
	OutColor = vec4(vec3(texture(Sampler, InUV).r), 1.0f);
#else
	OutColor = vec4(texture(Sampler, InUV).rgb, 1.0f);
#endif
}
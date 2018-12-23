#include "Common.glsl"

layout(location = 0) in vec2 InUV;
layout(location = 0) out vec4 OutColor;

uniform sampler2D Sampler;

void main()
{
	OutColor = vec4(texture(Sampler, InUV).rgb, 1.0f);
}
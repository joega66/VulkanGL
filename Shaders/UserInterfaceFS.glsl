#version 450

layout(binding = 0, set = 0) uniform sampler2D FontSampler;

layout(location = 0) in vec2 InUV;
layout(location = 1) in vec4 InColor;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = InColor * texture(FontSampler, InUV);
}
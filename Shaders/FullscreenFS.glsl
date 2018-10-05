#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 InUV;
layout(location = 0) out vec4 OutColor;

layout(binding = 0) uniform sampler2D Sampler;

void main()
{
	OutColor = vec4(texture(Sampler, InUV).rgb, 1.0f);
}
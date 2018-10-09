#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 OutColor;
layout(location = 0) in vec2 InUV;
layout(binding = 0) uniform sampler2D Image;

float radius = 0.50f;

void main()
{
	if (length(InUV - vec2(0.50f)) > radius)
		OutColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	else
		OutColor = texture(Image, InUV);
}
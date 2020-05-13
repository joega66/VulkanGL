#version 450

layout(location = 0) in vec2 InPos;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec4 InColor;

layout(push_constant) uniform PushConstants
{
	layout(offset = 0) vec4 _ScaleAndTranslate;
};

layout(location = 0) out vec2 OutUV;
layout(location = 1) out vec4 OutColor;

void main() 
{
	OutUV = InUV;
	OutColor = InColor;
	gl_Position = vec4(InPos * _ScaleAndTranslate.xy + _ScaleAndTranslate.zw, 0.0, 1.0);
}
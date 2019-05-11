#include "Common.glsl"

uniform ViewUniform
{
	mat4 WorldToView;
	mat4 ViewToClip;
	mat4 WorldToClip;
	vec3 Position;
	float Padding;
	float AspectRatio;
	float FieldOfView;
	vec2 MorePadding;
} View;
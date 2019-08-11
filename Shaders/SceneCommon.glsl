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

struct PointLight
{
	vec3 Position;
	float Intensity;
	vec3 Color;
	float Range;
};

layout(std430) buffer PointLightBuffer
{
	PointLight PointLights[];
};
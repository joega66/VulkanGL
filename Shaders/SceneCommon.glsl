#ifndef SCENE_COMMON
#define SCENE_COMMON

#define SCENE_SET 0

layout(binding = 0, set = SCENE_SET) uniform ViewUniform
{
	mat4 WorldToView;
	mat4 ViewToClip;
	mat4 WorldToClip;
	vec3 Position;
	float _Pad0;
	float AspectRatio;
	float FieldOfView;
	vec2 ScreenDims;
} View;

struct DirectionalLight
{
	vec3 Color;
	float Intensity;
	vec3 Direction;
	int _Pad1;
};

layout(std430, binding = 1, set = SCENE_SET) readonly buffer DirectionalLightBuffer
{
	uvec4 NumDirectionalLights;
	DirectionalLight DirectionalLights[];
};

struct PointLight
{
	vec3 Position;
	float Intensity;
	vec3 Color;
	float Range;
};

layout(std430, binding = 2, set = SCENE_SET) readonly buffer PointLightBuffer
{
	uvec4 NumPointLights;
	PointLight PointLights[];
};

#endif
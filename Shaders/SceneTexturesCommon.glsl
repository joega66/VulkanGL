#ifndef SCENE_TEXTURES_COMMON
#define SCENE_TEXTURES_COMMON

layout(binding = 0, set = SCENE_TEXTURES_SET) uniform sampler2D SceneDepth;
layout(binding = 1, set = SCENE_TEXTURES_SET) uniform sampler2D ShadowMask;

vec3 ScreenToWorld(vec2 UV)
{
	float Depth = texture(SceneDepth, UV).r;
	vec2 ClipSpace = (UV - 0.5f) * 2.0f;
	vec4 ClipSpaceH = vec4(ClipSpace, Depth, 1.0f);
	vec4 WorldSpace = Camera.ClipToWorld * ClipSpaceH;
	WorldSpace.xyz /= WorldSpace.w;
	return WorldSpace.xyz;
}

#endif
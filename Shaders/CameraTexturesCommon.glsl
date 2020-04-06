#ifndef CAMERA_TEXTURES_COMMON
#define CAMERA_TEXTURES_COMMON

layout(binding = 0, set = CAMERA_TEXTURES_SET) uniform sampler2D SceneDepth;

layout(binding = 1, set = CAMERA_TEXTURES_SET) uniform sampler3D RadianceVolume;

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
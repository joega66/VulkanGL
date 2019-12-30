#define SCENE_TEXTURES_SET 1
#define SHADOW_SET 2

#include "SceneCommon.glsl"
#include "SceneTexturesCommon.glsl"

layout(binding = 0, set = SHADOW_SET) uniform LightProjBuffer
{
	mat4 WorldToLight;
};

layout(binding = 1, set = SHADOW_SET) uniform sampler2D ShadowMap;

layout(location = 0) in vec2 InUV;
layout(location = 0) out vec4 OutColor;

void main()
{
	vec3 WorldSpace = ScreenToWorld(InUV);

	vec4 LightSpace = WorldToLight * vec4(WorldSpace, 1.0f);
	LightSpace.xyz /= LightSpace.w;
	LightSpace.xy = (LightSpace.xy + 1.0f) * 0.5f;

	float ShadowDepth = texture(ShadowMap, LightSpace.xy).r;
	float CurrentDepth = LightSpace.z;
	float ShadowFactor = CurrentDepth > ShadowDepth ? 1.0f : 0.0f;

	OutColor = vec4(ShadowFactor);
}
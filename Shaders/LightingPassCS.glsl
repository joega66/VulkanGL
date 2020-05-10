#include "CameraCommon.glsl"
#include "LightingCommon.glsl"
#define TEXTURE_SET 1
#define SAMPLER_SET 2
#include "SceneResources.glsl"

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1

layout(constant_id = 0) const uint _LIGHT_TYPE = 0;

layout(push_constant) uniform DirectionalLightConstants
{
	vec4 L;
	vec4 Radiance;
	mat4 LightViewProj;
	uint ShadowMap;
	uint ShadowMapSampler;
};

void GetDirectionalLightParams(inout LightParams Light)
{
	Light.L = L.xyz;
	Light.Radiance = Radiance.rgb;
}

void GetPointLightParams(inout LightParams Light, SurfaceData Surface)
{
	const vec3 FragToLight = L.xyz - Surface.WorldPosition;
	const float Distance = length(FragToLight);
	const float Attenuation = 1.0 / (Distance * Distance);

	Light.L = normalize(FragToLight);
	Light.Radiance = Radiance.rgb * Attenuation;
}

float ShadowPCF(vec3 WorldPosition)
{
	vec4 LightSpace = LightViewProj * vec4(WorldPosition, 1.0f);
	LightSpace.xyz /= LightSpace.w;
	LightSpace.xy = (LightSpace.xy + 1.0f) * 0.5f;
	float CurrentDepth = LightSpace.z;
	vec2 TexelSize = 1.0 / vec2( TextureSize(ShadowMap, 0) );

	float ShadowFactor = 0.0;

	for (int X = -1; X <= 1; X++)
	{
		for (int Y = -1; Y <= 1; Y++)
		{
			float ShadowDepth = Sample2D(ShadowMap, ShadowMapSampler, LightSpace.xy + vec2(X, Y) * TexelSize).r;
			float DepthTest = CurrentDepth > ShadowDepth ? 1.0f : 0.0f;
			ShadowFactor += DepthTest;
		}
	}

	ShadowFactor /= 9.0;
	
	return ( 1 - ShadowFactor);
}

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 SceneColorSize = imageSize(SceneColor);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, SceneColorSize.xy)))
		return;

	const ivec2 ScreenCoords = ivec2(gl_GlobalInvocationID.xy);
	const vec2 ScreenUV = vec2(ScreenCoords) / vec2(SceneColorSize);

	SurfaceData Surface;
	MaterialData Material;

	UnpackGBuffers(ScreenUV, ScreenCoords, Surface, Material);

	vec3 V = normalize(Camera.Position - Surface.WorldPosition);

	LightParams Light;

	if (_LIGHT_TYPE == DIRECTIONAL_LIGHT)
	{
		GetDirectionalLightParams(Light);
	}
	else if (_LIGHT_TYPE == POINT_LIGHT)
	{
		GetPointLightParams(Light, Surface);
	}

	vec3 Lo = vec3(0.0);

	Lo += DirectLighting(V, Light, Surface, Material);

	Lo *= ShadowPCF(Surface.WorldPosition);

	vec3 LoTotal = Lo + imageLoad(SceneColor, ScreenCoords).rgb;

	imageStore(SceneColor, ScreenCoords, vec4(LoTotal, 1.0));
}
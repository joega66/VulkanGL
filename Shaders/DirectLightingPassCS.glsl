#define CAMERA_SET 0
#include "CameraCommon.glsl"
#include "LightingCommon.glsl"
#define TEXTURE_SET 1
#include "SceneResources.glsl"

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1

layout(constant_id = 0) const uint _LIGHT_TYPE = 0;
layout(constant_id = 1) const int _IS_FIRST_LIGHT = 0;

layout(push_constant) uniform Params { DirectLightingParams _Params; };

void GetDirectionalLightParams(inout LightData light)
{
	light.l = _Params._L.xyz;
	light.radiance = _Params._Radiance.rgb;
}

void GetPointLightParams(inout LightData light, SurfaceData surface)
{
	const vec3 fragToLight = _Params._L.xyz - surface.worldPosition;
	const float distance = length(fragToLight);
	const float attenuation = 1.0 / (distance * distance);

	light.l = normalize(fragToLight);
	light.radiance = _Params._Radiance.rgb * attenuation;
}

float ShadowPCF(vec3 worldPosition)
{
	vec4 lightSpace = _Params._LightViewProj * vec4(worldPosition, 1.0f);
	lightSpace.xyz /= lightSpace.w;
	lightSpace.xy = (lightSpace.xy + 1.0f) * 0.5f;
	float currentDepth = lightSpace.z;
	vec2 texelSize = 1.0 / vec2( TextureSize(_Params._ShadowMap, 0) );

	float shadowFactor = 0.0;

	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			float shadowDepth = Sample2D(_Params._ShadowMap, lightSpace.xy + vec2(x, y) * texelSize).r;
			float depthTest = currentDepth > shadowDepth ? 1.0f : 0.0f;
			shadowFactor += depthTest;
		}
	}

	shadowFactor /= 9.0;
	
	return ( 1 - shadowFactor);
}

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 sceneColorSize = imageSize(_DirectLighting);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, sceneColorSize.xy)))
		return;

	const ivec2 screenCoords = ivec2(gl_GlobalInvocationID.xy);
	const vec2 screenUV = vec2(screenCoords) / vec2(sceneColorSize);

	SurfaceData surface;
	MaterialData material;

	UnpackGBuffers(screenUV, screenCoords, surface, material);

	vec3 v = normalize(_Camera.position - surface.worldPosition);

	LightData light;

	if (_LIGHT_TYPE == DIRECTIONAL_LIGHT)
	{
		GetDirectionalLightParams(light);
	}
	else if (_LIGHT_TYPE == POINT_LIGHT)
	{
		GetPointLightParams(light, surface);
	}

	vec3 ld = vec3(0.0);

	ld += DirectLighting(v, light, surface, material);

	ld *= ShadowPCF(surface.worldPosition);

	vec3 ldTotal = ld;

	if ( _IS_FIRST_LIGHT == 0 )
	{
		ldTotal += imageLoad(_DirectLighting, screenCoords).rgb;
	}

	imageStore(_DirectLighting, screenCoords, vec4(ldTotal, 1.0));
}
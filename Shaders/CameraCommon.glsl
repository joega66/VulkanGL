#ifndef CAMERA_COMMON
#define CAMERA_COMMON
#include "SurfaceInterface.glsl"
#include "MaterialInterface.glsl"
#include "LightingCommon.glsl"

#define CAMERA_SET 0
layout(binding = 0, set = CAMERA_SET) uniform CameraUniform
{
	mat4 worldToView;
	mat4 viewToClip;
	mat4 worldToClip;
	mat4 clipToWorld;
	vec3 position;
	float _pad0;
	float aspectRatio;
	float fieldOfView;
	vec2 screenDims;
	vec3 clipData;
	float _pad1;
} _Camera;
layout(binding = 1, set = CAMERA_SET) uniform sampler2D _SceneDepth;
layout(binding = 2, set = CAMERA_SET) uniform sampler2D _GBuffer0;
layout(binding = 3, set = CAMERA_SET) uniform sampler2D _GBuffer1;
layout(binding = 4, set = CAMERA_SET, rgba16f) uniform image2D _SceneColor;
layout(binding = 5, set = CAMERA_SET, rgba16f) uniform image2D _SSGIHistory;

/** Transform from screen space to world space. */
vec3 ScreenToWorld(vec2 screenUV)
{
	float depth = texture(_SceneDepth, screenUV).r;
	vec2 clipSpace = (screenUV - 0.5f) * 2.0f;
	vec4 clipSpaceH = vec4(clipSpace, depth, 1.0f);
	vec4 worldSpace = _Camera.clipToWorld * clipSpaceH;
	worldSpace.xyz /= worldSpace.w;
	return worldSpace.xyz;
}

/** Returns the surface and material at a screen coordinate. */
void UnpackGBuffers(vec2 screenUV, ivec2 screenCoords, inout SurfaceData surface, inout MaterialData material)
{
	const vec4 gBuffer0Data = texelFetch(_GBuffer0, screenCoords, 0);
	const vec4 gBuffer1Data = texelFetch(_GBuffer1, screenCoords, 0);

	surface.worldPosition = ScreenToWorld(screenUV);
	surface.worldNormal = gBuffer0Data.rgb;

	material.baseColor = gBuffer1Data.rgb;
	material.metallic = gBuffer0Data.a;
	material.roughness = gBuffer1Data.a;
	material.specularColor = mix(vec3(0.04), material.baseColor, material.metallic);
	material.diffuseColor = Diffuse_BRDF(material.baseColor);
}

vec3 LoadNormal(ivec2 screenCoords)
{
	return texelFetch(_GBuffer0, screenCoords, 0).rgb;
}

#endif
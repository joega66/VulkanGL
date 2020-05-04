#ifndef CAMERA_COMMON
#define CAMERA_COMMON
#include "SurfaceInterface.glsl"
#include "MaterialInterface.glsl"
#include "LightingCommon.glsl"

#define CAMERA_SET 0
layout(binding = 0, set = CAMERA_SET) uniform CameraUniform
{
	mat4 WorldToView;
	mat4 ViewToClip;
	mat4 WorldToClip;
	mat4 ClipToWorld;
	vec3 Position;
	float _Pad0;
	float AspectRatio;
	float FieldOfView;
	vec2 ScreenDims;
} Camera;
layout(binding = 1, set = CAMERA_SET) uniform sampler2D SceneDepth;
layout(binding = 2, set = CAMERA_SET) uniform sampler2D GBuffer0;
layout(binding = 3, set = CAMERA_SET) uniform sampler2D GBuffer1;
layout(binding = 4, set = CAMERA_SET) uniform sampler3D RadianceVolume;
layout(binding = 5, set = CAMERA_SET, rgba8) uniform image2D SceneColor;

/** Transform from screen space to world space. */
vec3 ScreenToWorld(vec2 ScreenUV)
{
	float Depth = texture(SceneDepth, ScreenUV).r;
	vec2 ClipSpace = (ScreenUV - 0.5f) * 2.0f;
	vec4 ClipSpaceH = vec4(ClipSpace, Depth, 1.0f);
	vec4 WorldSpace = Camera.ClipToWorld * ClipSpaceH;
	WorldSpace.xyz /= WorldSpace.w;
	return WorldSpace.xyz;
}

/** Returns the Surface and Material at a screen coordinate. */
void UnpackGBuffers(vec2 ScreenUV, ivec2 ScreenCoords, inout SurfaceData Surface, inout MaterialData Material)
{
	const vec4 GBuffer0Data = texelFetch(GBuffer0, ScreenCoords, 0);
	const vec4 GBuffer1Data = texelFetch(GBuffer1, ScreenCoords, 0);

	Surface.WorldPosition = ScreenToWorld(ScreenUV);
	Surface.WorldNormal = GBuffer0Data.rgb;

	Material.BaseColor = GBuffer1Data.rgb;
	Material.Metallic = GBuffer0Data.a;
	Material.Roughness = GBuffer1Data.a;
	Material.SpecularColor = mix(vec3(0.04), Material.BaseColor, Material.Metallic);
	Material.DiffuseColor = Diffuse_BRDF(Material.BaseColor);
}

#endif
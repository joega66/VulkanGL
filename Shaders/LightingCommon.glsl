#ifndef LIGHTING_COMMON
#define LIGHTING_COMMON

const float PI = 3.14159265;
const float AMBIENT = 0.01f;

#ifdef SCENE_TEXTURES_SET
#include "SceneTexturesCommon.glsl"
#endif

#ifdef TRACE_SHADOW_CONE
#include "VoxelsCommon.glsl"
float TraceShadowCone(in vec3 WorldPosition, in vec3 WorldNormal, in vec3 LightDir)
{
	return texture(RadianceVolume, TransformWorldToVoxelUVW(WorldPosition + WorldNormal * VOXEL_SIZE)).a;
	//const float Aperture = 0.03f;
	//const float SamplingFactor = 0.5f;
	//const vec3 StartPosition = WorldPosition + WorldNormal * VOXEL_SIZE;

	//vec3 VolumeUV = TransformWorldToVoxelUVW(StartPosition);

	//float Occlusion = 0.0;
	//float Dist = VOXEL_SIZE;

	//while (Occlusion < 1.0 && Dist < 1.0)
	//{
	//	const float Diameter = 2.0f * Aperture * Dist;
	//	Occlusion += texture(RadianceVolume, VolumeUV + LightDir * Dist).a;
	//	Dist += VOXEL_SIZE;
	//	//Dist += Diameter * SamplingFactor;
	//}

	//return clamp(Occlusion, 0.0f, 1.0f);
}
#endif

vec3 ApplyGammaCorrection(in vec3 Color)
{
	Color = Color / (Color + vec3(1.0));
	Color = pow(Color, vec3(1.0 / 2.2));
	return Color;
}

struct LightParams
{
	vec3 L;
	vec3 Radiance;
};

/* BRDF */

/*
 * Approximates the Fresnel effect, the increase in reflectance at glancing
 * angles.
 */
vec3 FresnelSchlick(float NdotL, vec3 R0)
{
	return R0 + (1.0 - R0) * pow(1.0 - NdotL, 5.0);
}

/* 
 * The normal distribution function (NDF) defines the distribution of the 
 * microfacet surface normals. 
 */
float TrowbridgeReitzNDF(float NdotH, float Roughness)
{
	/* AlphaTr = s^2, where s is an artist-manipulated value between 0 and 1. */
	float AlphaTr = Roughness * Roughness;
	float Nom = AlphaTr * AlphaTr;
	float NdotH2 = NdotH * NdotH;
	float Denom = (NdotH2 * (Nom - 1.0) + 1.0);
	Denom = PI * Denom * Denom;

	return Nom / Denom;
}

/* 
 * The geometry function is equal to the possibility that a ray of light in
 * direction L will be reflected into direction V without being shadowed or 
 * masked. 
 */
float SchlickGF(float NdotV, float Roughness)
{
	float R = Roughness + 1.0;
	float K = R * R / 8.0;
	float Nom = NdotV;
	float Denom = NdotV * (1.0 - K) + K;

	return Nom / Denom;
}

float SmithGF(float NdotV, float NdotL, float Roughness)
{
	float Geom1 = SchlickGF(NdotV, Roughness);
	float Geom2 = SchlickGF(NdotL, Roughness);

	return Geom1 * Geom2;
}

vec3 DirectLighting(in vec3 V, in LightParams Light, in SurfaceData Surface, in MaterialData Material, in vec3 R0)
{
	float NdotV = max(dot(Surface.WorldNormal, V), 0.0);
	vec3 H = normalize(Light.L + V);

	float NdotL = max(dot(Surface.WorldNormal, Light.L), 0.0);
	float NdotH = max(dot(Surface.WorldNormal, H), 0.0);

	vec3 Fresnel = FresnelSchlick(NdotL, R0);
	float NDF = TrowbridgeReitzNDF(NdotH, Material.Roughness);
	float G = SmithGF(NdotV, NdotL, Material.Roughness);

	vec3 Nom = NDF * G * Fresnel;
	float Denom = 4.0 * NdotV * NdotL;
	vec3 Specular = Nom / max(Denom, 0.001);

	vec3 Kd = vec3(1.0) - Fresnel;
	Kd *= 1.0 - Material.Metallicity;

	vec3 Lo = (Kd * Material.BaseColor / PI + Specular) * Light.Radiance * NdotL;

	return Lo;
}

vec4 Shade(in SurfaceData Surface, in MaterialData Material)
{
	vec3 Lo = vec3(0.0);
	vec3 V = normalize(Camera.Position - Surface.WorldPosition);
	vec3 R0 = vec3(0.04);
	R0 = mix(R0, Material.BaseColor, Material.Roughness);

	// Directional lights
	for (int LightIndex = 0; LightIndex < NumDirectionalLights.x; LightIndex++)
	{
		LightParams Light;
		Light.L = DirectionalLights[LightIndex].Direction;
		Light.Radiance = DirectionalLights[LightIndex].Intensity * DirectionalLights[LightIndex].Color;

		vec3 Radiance = DirectLighting(V, Light, Surface, Material, R0);

#ifdef TRACE_SHADOW_CONE
		Lo += Radiance * TraceShadowCone(Surface.WorldPosition, Surface.WorldNormal, Light.L);
#else
		Lo += Radiance;
#endif
	}

	// Point lights
	for (int LightIndex = 0; LightIndex < NumPointLights.x; LightIndex++)
	{
		vec3 FragToLight = PointLights[LightIndex].Position - Surface.WorldPosition;
		float Distance = length(FragToLight);
		float Attenuation = 1.0 / (Distance * Distance);

		LightParams Light;
		Light.L = normalize(FragToLight);
		Light.Radiance = PointLights[LightIndex].Intensity * PointLights[LightIndex].Color * Attenuation;

		Lo += DirectLighting(V, Light, Surface, Material, R0);
	}

	return vec4(Lo, Material.Alpha);
}

#endif
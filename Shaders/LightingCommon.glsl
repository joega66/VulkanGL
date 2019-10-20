#ifndef LIGHTING_COMMON
#define LIGHTING_COMMON

const float PI = 3.14159265;
const float AMBIENT = 0.01f;

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

vec3 DirectLighting(in vec3 V, in LightParams Light, in MaterialParams Material, in vec3 R0)
{
	float NdotV = max(dot(Material.Normal, V), 0.0);
	vec3 H = normalize(Light.L + V);

	float NdotL = max(dot(Material.Normal, Light.L), 0.0);
	float NdotH = max(dot(Material.Normal, H), 0.0);

	vec3 Fresnel = FresnelSchlick(NdotL, R0);
	float NDF = TrowbridgeReitzNDF(NdotH, Material.Roughness);
	float G = SmithGF(NdotV, NdotL, Material.Roughness);

	vec3 Nom = NDF * G * Fresnel;
	float Denom = 4.0 * NdotV * NdotL;
	vec3 Specular = Nom / max(Denom, 0.001);
	Specular *= Material.Specular;

	vec3 Kd = vec3(1.0) - Fresnel;
	Kd *= 1.0 - Material.Shininess;

	vec3 Lo = (Kd * Material.Albedo / PI + Specular) * Light.Radiance * NdotL; // * Shadow

	return Lo;
}

vec4 Shade(in MaterialParams Material)
{
	vec3 Lo = vec3(0.0);
	vec3 V = normalize(View.Position - Material.Position);
	vec3 R0 = vec3(0.04);
	R0 = mix(R0, Material.Albedo, Material.Shininess);

	// Directional lights
	for (int LightIndex = 0; LightIndex < NumDirectionalLights.x; LightIndex++)
	{
		LightParams Light;
		Light.L = DirectionalLights[LightIndex].Direction;
		Light.Radiance = DirectionalLights[LightIndex].Intensity * DirectionalLights[LightIndex].Color;

		Lo += DirectLighting(V, Light, Material, R0);
	}

	// Point lights
	for (int LightIndex = 0; LightIndex < NumPointLights.x; LightIndex++)
	{
		vec3 FragToLight = PointLights[LightIndex].Position - Material.Position;
		float Distance = length(FragToLight);
		float Attenuation = 1.0 / (Distance * Distance);

		LightParams Light;
		Light.L = normalize(FragToLight);
		Light.Radiance = PointLights[LightIndex].Intensity * PointLights[LightIndex].Color * Attenuation;

		Lo += DirectLighting(V, Light, Material, R0);
	}

	// Ambient
	vec3 Ambient = vec3(AMBIENT) * Material.Albedo; // * AO
	Lo += Ambient;

	// Gamma
	Lo = Lo / (Lo + vec3(1.0));
	Lo = pow(Lo, vec3(1.0 / 2.2));

	return vec4(Lo, Material.Alpha);
}

#endif
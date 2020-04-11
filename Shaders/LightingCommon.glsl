#ifndef LIGHTING_COMMON
#define LIGHTING_COMMON

const float PI = 3.14159265;

#ifdef TRACE_DIFFUSE_CONES
vec3 TraceDiffuseGI(vec3 WorldPosition, vec3 WorldNormal)
{
	const float ConeAngle = 60.0;
	const float Aperture = atan(radians(ConeAngle / 2.0));
	const vec3 StartPosition = WorldPosition + WorldNormal * VOXEL_SIZE;
	const vec3 VolumeUV = TransformWorldToVoxelUVW(StartPosition);

	float Dist = VOXEL_SIZE;
	vec3 DiffuseGI = vec3(0.0);
	float Alpha = 0.0;

	while (Alpha < 1.0 && Dist < 1.0)
	{
		const float Diameter = 2.0 * Aperture * Dist;
		const float MipLevel = log2(Diameter * VOXEL_SIZE);
		const vec4 RadianceSample = textureLod(RadianceVolume, VolumeUV + WorldNormal * Dist, MipLevel).rgba;
		DiffuseGI = Alpha * DiffuseGI + (1.0 - Alpha) * RadianceSample.a * RadianceSample.rgb; // c = a * c + (1 - a)a2 * c
		Alpha += (1.0 - Alpha) * RadianceSample.a; // alpha = alpha + (1 - alpha)alpha2
		Dist += Diameter;
	}

	return DiffuseGI;
}
#endif

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
vec3 FresnelSchlick(vec3 SpecularColor, float NdotL)
{
	return SpecularColor + (1.0 - SpecularColor) * pow(1.0 - NdotL, 5.0);
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

vec3 DirectLighting(vec3 V, LightParams Light, SurfaceData Surface, MaterialData Material)
{
	float NdotV = max(dot(Surface.WorldNormal, V), 0.0);
	vec3 H = normalize(Light.L + V);

	float NdotL = max(dot(Surface.WorldNormal, Light.L), 0.0);
	float NdotH = max(dot(Surface.WorldNormal, H), 0.0);

	vec3 Fresnel = FresnelSchlick(Material.SpecularColor, NdotL);
	float NDF = TrowbridgeReitzNDF(NdotH, Material.Roughness);
	float G = SmithGF(NdotV, NdotL, Material.Roughness);

	vec3 Nom = NDF * G * Fresnel;
	float Denom = 4.0 * NdotV * NdotL;
	vec3 Specular = Nom / max(Denom, 0.001);

	vec3 Lo = (Material.BaseColor / PI + Specular) * Light.Radiance * NdotL;

	return Lo;
}

#endif
#ifndef LIGHTING_COMMON
#define LIGHTING_COMMON

const float PI = 3.14159265;

struct LightData
{
	vec3 L;
	vec3 Radiance;
};

/* BRDF */

struct BRDFContext
{
	float NdotL;
	float NdotH;
	float NdotV;
};

void BRDF_InitContext(out BRDFContext Context, vec3 N, vec3 V, vec3 L)
{
	vec3 H = normalize(L + V);

	Context.NdotL = max(dot(N, L), 0.0);
	Context.NdotH = max(dot(N, H), 0.0);
	Context.NdotV = max(dot(N, V), 0.0);
}

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

vec3 Specular_BRDF(BRDFContext Context, vec3 SpecularColor, float Roughness)
{
	vec3 Fresnel = FresnelSchlick(SpecularColor, Context.NdotL);
	float NDF = TrowbridgeReitzNDF(Context.NdotH, Roughness);
	float G = SmithGF(Context.NdotV, Context.NdotL, Roughness);

	vec3 Nom = NDF * G * Fresnel;
	float Denom = 4.0 * Context.NdotV * Context.NdotL;
	vec3 Specular = Nom / max(Denom, 0.001);

	return Specular;
}

vec3 Diffuse_BRDF(vec3 BaseColor)
{
	return BaseColor / PI;
}

vec3 DirectLighting(vec3 V, LightData Light, SurfaceData Surface, MaterialData Material)
{
	BRDFContext BRDFContext;
	BRDF_InitContext(BRDFContext, Surface.WorldNormal, V, Light.L);

	vec3 Lo = ( Material.DiffuseColor + Specular_BRDF(BRDFContext, Material.SpecularColor, Material.Roughness) ) * Light.Radiance * BRDFContext.NdotL;
	
	return Lo;
}

#endif
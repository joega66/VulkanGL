#ifndef LIGHTING_COMMON
#define LIGHTING_COMMON

const float PI = 3.14159265;

struct LightData
{
	vec3 l;
	vec3 radiance;
};

/* BRDF */

struct BRDFContext
{
	float ndotl;
	float ndoth;
	float ndotv;
};

void BRDF_InitContext(out BRDFContext brdf, vec3 n, vec3 v, vec3 l)
{
	vec3 h = normalize(l + v);

	brdf.ndotl = max(dot(n, l), 0.0);
	brdf.ndoth = max(dot(n, h), 0.0);
	brdf.ndotv = max(dot(n, v), 0.0);
}

/*
 * Approximates the Fresnel effect, the increase in reflectance at glancing
 * angles.
 */
vec3 FresnelSchlick(vec3 specularColor, float ndotl)
{
	return specularColor + (1.0 - specularColor) * pow(1.0 - ndotl, 5.0);
}

/* 
 * The (Trowbridge-Reitz) normal distribution function (NDF) defines the distribution of the 
 * microfacet surface normals.
 */
float NormalGGX(float ndoth, float alpha)
{
	float nom = alpha * alpha;
	float ndoth2 = ndoth * ndoth;
	float denom = (ndoth2 * (nom - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

/* 
 * The (Schlick) geometry function is equal to the possibility that a ray of light in
 * direction l will be reflected into direction v without being shadowed or 
 * masked.
 */
float GeometryGGX(float ndotv, float k)
{
	float nom = ndotv;
	float denom = ndotv * (1.0 - k) + k;

	return nom / denom;
}

float SmithGF(float ndotv, float ndotl, float roughness)
{
	float k = pow(roughness + 1.0, 2.0) / 8.0;
	float geom1 = GeometryGGX(ndotv, k);
	float geom2 = GeometryGGX(ndotl, k);

	return geom1 * geom2;
}

vec3 Specular_BRDF(BRDFContext brdf, vec3 specularColor, float roughness, out vec3 fresnel)
{
	roughness *= roughness;

	fresnel = FresnelSchlick(specularColor, brdf.ndotl);
	float ndf = NormalGGX(brdf.ndoth, roughness);
	float g = SmithGF(brdf.ndotv, brdf.ndotl, roughness);

	vec3 nom = ndf * g * fresnel;
	float denom = 4.0 * brdf.ndotv * brdf.ndotl;
	vec3 specular = nom / max(denom, 0.001);

	return specular;
}

vec3 Diffuse_BRDF(vec3 baseColor)
{
	return baseColor / PI;
}

vec3 DirectLighting(vec3 v, LightData light, SurfaceData surface, MaterialData material)
{
	BRDFContext brdf;
	BRDF_InitContext(brdf, surface.worldNormal, v, light.l);

	vec3 fresnel;
	const vec3 specularColor = Specular_BRDF(brdf, material.specularColor, material.roughness, fresnel);

	const vec3 lo = mix( material.diffuseColor, specularColor, fresnel ) * light.radiance * brdf.ndotl;
	
	return lo;
}

#endif
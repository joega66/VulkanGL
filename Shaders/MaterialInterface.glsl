#ifndef MATERIAL_INTERFACE
#define MATERIAL_INTERFACE

struct MaterialData
{
	vec3 baseColor;
	vec3 specularColor;
	vec3 diffuseColor;
	float metallic;
	float roughness;
};

#endif
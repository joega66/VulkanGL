#ifndef MATERIAL_INTERFACE
#define MATERIAL_INTERFACE

struct MaterialData
{
	vec3 BaseColor;
	vec3 SpecularColor;
	float Alpha;
	float Metallic;
	float Roughness;
};

#endif
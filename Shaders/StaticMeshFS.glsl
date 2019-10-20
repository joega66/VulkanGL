#ifndef STATIC_MESH_FS
#define STATIC_MESH_FS
#include "MaterialCommon.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec3 InNormal;
layout(location = 3) in vec3 InTangent;

MaterialParams GetMaterial()
{
	MaterialParams Material;
	Material.Position = InPosition;
	Material.Normal = normalize(InNormal);
	Material.Albedo = texture(Diffuse, InUV).rgb;
	Material.Roughness = 0.25f;
	Material.Shininess = 0.0f;

	if (HasSpecularMap)
	{
		Material.Specular = texture(Specular, InUV).rgb;
	}
	else
	{
		Material.Specular = vec3(1.0f);
	}

	if (HasOpacityMap)
	{
		Material.Alpha = texture(Opacity, InUV).r;
	}
	else
	{
		Material.Alpha = 1.0f;
	}

	return Material;
}

#endif
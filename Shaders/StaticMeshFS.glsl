#include "MaterialCommon.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec3 InNormal;

MaterialParams GetMaterial()
{
	MaterialParams Material;
	Material.Position = InPosition;
	Material.Normal = InNormal;
	Material.Albedo = texture(Diffuse, InUV).rgb;

	if (HasSpecularMap)
	{
		Material.Specular = texture(Specular, InUV).rgb;
	}
	else
	{
		Material.Specular = vec3(1.0f);
	}

	Material.Roughness = 0.25f;
	Material.Shininess = 0.0f;

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
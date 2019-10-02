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
	Material.Metallic = 1.0f;

	return Material;
}
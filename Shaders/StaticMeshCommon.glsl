#ifndef STATIC_MESH_COMMON
#define STATIC_MESH_COMMON

#include "MaterialCommon.glsl"

#if GEOMETRY_SHADER
layout(location = 0) in vec4 InPosition[];
layout(location = 1) in vec2 InUV[];
layout(location = 2) in vec3 InNormal[];
layout(location = 3) in vec3 InTangent[];
#elif VERTEX_SHADER
layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec3 InNormal;
layout(location = 3) in vec3 InTangent;
#else
layout(location = 0) in vec4 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec3 InNormal;
layout(location = 3) in vec3 InTangent;
#endif

#if !FRAGMENT_SHADER
layout(location = 0) out vec4 OutPosition;
layout(location = 1) out vec2 OutUV;
layout(location = 2) out vec3 OutNormal;
layout(location = 3) out vec3 OutTangent;
#endif

#if VERTEX_SHADER

vec4 GetWorldPosition()
{
	return LocalToWorld.Transform * vec4(InPosition, 1.0f);
}

void SetVSInterpolants(in vec4 WorldPosition)
{
	OutPosition = WorldPosition;
	OutUV = InUV;
	OutNormal = mat3(transpose(LocalToWorld.Inverse)) * InNormal;
	OutTangent = InTangent;
}

#elif GEOMETRY_SHADER

void SetGSInterpolants(in uint VertexIndex)
{
	OutPosition = InPosition[VertexIndex];
	OutUV = InUV[VertexIndex];
	OutNormal = InNormal[VertexIndex];
	OutTangent = InTangent[VertexIndex];
}

#endif

#if FRAGMENT_SHADER

MaterialParams GetMaterial()
{
	MaterialParams Material;
	Material.Position = InPosition.xyz;
	Material.Normal = normalize(InNormal);
	Material.Albedo = texture(Diffuse, InUV).rgb;
	Material.Roughness = 0.25f;
	Material.Shininess = 0.0f;

	if (HasOpacityMap)
	{
		Material.Alpha = texture(Opacity, InUV).r;
		if (Material.Alpha <= 0)
		{
			discard;
		}
	}
	else
	{
		Material.Alpha = 1.0f;
	}

	if (HasSpecularMap)
	{
		Material.Specular = texture(Specular, InUV).rgb;
	}
	else
	{
		Material.Specular = vec3(1.0f);
	}

	return Material;
}

#endif

#endif
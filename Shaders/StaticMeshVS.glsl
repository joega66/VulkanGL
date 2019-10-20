#ifndef STATIC_MESH_VS
#define STATIC_MESH_VS
#include "MaterialCommon.glsl"

#ifdef GEOMETRY_SHADER
layout(location = 0) in vec3 Position[];
layout(location = 1) in vec2 UV[];
layout(location = 2) in vec3 Normal[];
layout(location = 3) in vec3 Tangent[];
#else
layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
#endif

layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec2 OutUV;
layout(location = 2) out vec3 OutNormal;
layout(location = 3) out vec3 OutTangent;

#ifdef GEOMETRY_SHADER

void SetGSInterpolants(in uint VertexIndex)
{
	OutPosition = Position[VertexIndex];
	OutUV = UV[VertexIndex];
	OutNormal = Normal[VertexIndex];
	OutTangent = Tangent[VertexIndex];
}

#else

vec4 GetWorldPosition()
{
	return LocalToWorld.Transform * vec4(Position, 1.0f);
}

void SetVSInterpolants(in vec4 WorldPosition)
{
	OutPosition = WorldPosition.xyz;
	OutUV = UV;
	OutNormal = mat3(transpose(inverse(LocalToWorld.Transform))) * Normal;
	OutTangent = Tangent;
}

#endif

#endif
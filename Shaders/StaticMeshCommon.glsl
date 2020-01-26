#ifndef STATIC_MESH_COMMON
#define STATIC_MESH_COMMON

#define MESH_SET 1

//struct SurfaceOutput
//{
//	vec3 Position;
//	vec2 UV;
//	vec3 Normal;
//	vec3 Tangent;
//};

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

layout(binding = 0, set = MESH_SET) uniform LocalToWorldUniform
{
	mat4 Transform;
	mat4 Inverse;
} LocalToWorld;

#if VERTEX_SHADER

vec4 Surface_GetWorldPosition()
{
	return LocalToWorld.Transform * vec4(InPosition, 1.0f);
}

void Surface_SetAttributes(in vec4 WorldPosition)
{
	OutPosition = WorldPosition;
	OutUV = InUV;
	OutNormal = mat3(transpose(LocalToWorld.Inverse)) * InNormal;
	OutTangent = InTangent;
}

#elif GEOMETRY_SHADER

void Surface_SetAttributes(in uint VertexIndex)
{
	OutPosition = InPosition[VertexIndex];
	OutUV = InUV[VertexIndex];
	OutNormal = InNormal[VertexIndex];
	OutTangent = InTangent[VertexIndex];
}

SurfaceData Surface_Get(in uint VertexIndex)
{
	SurfaceData Surface;
	Surface.WorldPosition = InPosition[VertexIndex].xyz;
	Surface.WorldNormal = InNormal[VertexIndex];
	Surface.UV = InUV[VertexIndex];
	return Surface;
}

#elif FRAGMENT_SHADER

SurfaceData Surface_Get()
{
	SurfaceData Surface;
	Surface.WorldPosition = InPosition.xyz;
	Surface.WorldNormal = normalize(InNormal);
	Surface.UV = InUV;
	return Surface;
}

#endif

#endif
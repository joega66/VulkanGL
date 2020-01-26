#ifndef STATIC_MESH_COMMON
#define STATIC_MESH_COMMON

#define SURFACE(InOut)						\
layout(location = 0) InOut Surface##InOut	\
{											\
	vec3 Position;							\
	vec2 UV;								\
	vec3 Normal;							\
	vec3 Tangent;							\
} 											\

#if VERTEX_SHADER
layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
#elif GEOMETRY_SHADER
SURFACE(in) Input[];
#else
SURFACE(in) Input;
#endif

#if !FRAGMENT_SHADER
SURFACE(out) Output;
#endif

#define MESH_SET 1

layout(binding = 0, set = MESH_SET) uniform LocalToWorldUniform
{
	mat4 Transform;
	mat4 Inverse;
} LocalToWorld;

#if VERTEX_SHADER

vec4 Surface_GetWorldPosition()
{
	return LocalToWorld.Transform * vec4(Position, 1.0f);
}

void Surface_SetAttributes(in vec4 WorldPosition)
{
	Output.Position = WorldPosition.xyz;
	Output.UV = UV;
	Output.Normal = mat3(transpose(LocalToWorld.Inverse)) * Normal;
	Output.Tangent = Tangent;
}

#elif GEOMETRY_SHADER

void Surface_SetAttributes(in uint VertexIndex)
{
	Output.Position = Input[VertexIndex].Position;
	Output.UV = Input[VertexIndex].UV;
	Output.Normal = Input[VertexIndex].Normal;
	Output.Tangent = Input[VertexIndex].Tangent;
}

SurfaceData Surface_Get(in uint VertexIndex)
{
	SurfaceData Surface;
	Surface.WorldPosition = Input[VertexIndex].Position;
	Surface.WorldNormal = Input[VertexIndex].Normal;
	Surface.UV = Input[VertexIndex].UV;
	return Surface;
}

#elif FRAGMENT_SHADER

SurfaceData Surface_Get()
{
	SurfaceData Surface;
	Surface.WorldPosition = Input.Position;
	Surface.WorldNormal = normalize(Input.Normal);
	Surface.UV = Input.UV;
	return Surface;
}

#endif

#endif
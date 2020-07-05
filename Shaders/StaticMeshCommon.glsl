#ifndef STATIC_MESH_COMMON
#define STATIC_MESH_COMMON

#define SURFACE(InOut)						\
layout(location = 0) InOut Surface##InOut	\
{											\
	vec3 position;							\
	vec2 uv;								\
	vec3 normal;							\
} 											\

#if VERTEX_SHADER
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
#elif GEOMETRY_SHADER
SURFACE(in) inSurface[];
#else
SURFACE(in) inSurface;
#endif

#if !FRAGMENT_SHADER
SURFACE(out) outSurface;
#endif

#ifdef MESH_SET

layout(binding = 0, set = MESH_SET) uniform LocalToWorldUniform
{
	mat4 transform;
	mat4 inverse;
	mat4 inverseTranspose;
} _LocalToWorld;

#endif

#if VERTEX_SHADER

vec4 Surface_GetWorldPosition()
{
	return _LocalToWorld.transform * vec4(position, 1.0f);
}

void Surface_SetAttributes(in vec4 worldPosition)
{
	outSurface.position = worldPosition.xyz;
	outSurface.uv = uv;
	outSurface.normal = mat3(_LocalToWorld.inverseTranspose) * normal;
}

#elif GEOMETRY_SHADER

void Surface_SetAttributes(in uint vertexIndex)
{
	outSurface.position = inSurface[vertexIndex].position;
	outSurface.uv = inSurface[vertexIndex].uv;
	outSurface.normal = inSurface[vertexIndex].normal;
}

SurfaceData Surface_Get(in uint vertexIndex)
{
	SurfaceData surface;
	surface.worldPosition = inSurface[vertexIndex].position;
	surface.worldNormal = inSurface[vertexIndex].normal;
	surface.uv = inSurface[vertexIndex].uv;
	return surface;
}

#elif FRAGMENT_SHADER

SurfaceData Surface_Get()
{
	SurfaceData surface;
	surface.worldPosition = inSurface.position;
	surface.worldNormal = normalize(inSurface.normal);
	surface.uv = inSurface.uv;
	return surface;
}

#endif

#endif
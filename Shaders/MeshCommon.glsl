#ifndef MESH_COMMON
#define MESH_COMMON

struct SurfaceData
{
	vec3 WorldPosition;
	vec3 WorldNormal;
	vec2 UV;
};

#ifdef STATIC_MESH
#include "StaticMeshCommon.glsl"
#endif

#endif
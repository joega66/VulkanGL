#include "View.glsl"
#ifdef STATIC_MESH
#include "StaticMeshVS.glsl"
#endif

void main()
{
	vec4 WorldPosition = GetWorldPosition();
	gl_Position = View.WorldToClip * WorldPosition;
}
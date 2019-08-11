#include "SceneCommon.glsl"
#ifdef STATIC_MESH
#include "StaticMeshVS.glsl"
#endif

void main()
{
	vec4 WorldPosition = GetWorldPosition();

	SetVSInterpolants(WorldPosition);

	gl_Position = View.WorldToClip * WorldPosition;
}
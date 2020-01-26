#include "SceneCommon.glsl"
#include "MeshCommon.glsl"

void main()
{
	vec4 WorldPosition = Surface_GetWorldPosition();

	Surface_SetAttributes(WorldPosition);

	gl_Position = View.WorldToClip * WorldPosition;
}
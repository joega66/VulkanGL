#include "CameraCommon.glsl"
#include "MeshCommon.glsl"

void main()
{
	vec4 WorldPosition = Surface_GetWorldPosition();

	Surface_SetAttributes(WorldPosition);

	gl_Position = Camera.WorldToClip * WorldPosition;
}
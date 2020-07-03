#include "CameraCommon.glsl"
#include "MeshCommon.glsl"

void main()
{
	vec4 worldPosition = Surface_GetWorldPosition();

	Surface_SetAttributes(worldPosition);

	gl_Position = _Camera.worldToClip * worldPosition;
}
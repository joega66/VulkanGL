#define CAMERA_SET 0
#include "CameraCommon.glsl"
#define MESH_SET 1
#include "MeshCommon.glsl"

void main()
{
	vec4 worldPosition = Surface_GetWorldPosition();

	Surface_SetAttributes(worldPosition);

	gl_Position = _Camera.worldToClip * worldPosition;
}
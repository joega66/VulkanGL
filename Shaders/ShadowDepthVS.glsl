#define MESH_SET 1
#include "MeshCommon.glsl"

layout(binding = 0, set = 0) uniform ShadowUniformBlock { ShadowUniform _Shadow; };

void main()
{
	vec4 worldPosition = Surface_GetWorldPosition();

	Surface_SetAttributes(worldPosition);

	gl_Position = _Shadow.lightViewProj * worldPosition;
}
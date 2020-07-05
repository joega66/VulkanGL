#define MESH_SET 1
#include "MeshCommon.glsl"

layout(binding = 0, set = 0) uniform LightViewProjBuffer
{
	mat4 _LightViewProj;
	mat4 _InvLightViewProj;
};

void main()
{
	vec4 worldPosition = Surface_GetWorldPosition();

	Surface_SetAttributes(worldPosition);

	gl_Position = _LightViewProj * worldPosition;
}
#include "MeshCommon.glsl"

layout(binding = 0, set = 0) uniform LightViewProjBuffer
{
	mat4 LightViewProj;
	mat4 InvLightViewProj;
};

void main()
{
	vec4 WorldPosition = Surface_GetWorldPosition();

	Surface_SetAttributes(WorldPosition);

	gl_Position = LightViewProj * WorldPosition;
}
#include "MeshCommon.glsl"

layout(binding = 0, set = 0) uniform LightViewProjBuffer
{
	mat4 LightViewProj;
};

void main()
{
	vec4 WorldPosition = GetWorldPosition();

	SetVSInterpolants(WorldPosition);

	gl_Position = LightViewProj * WorldPosition;
}
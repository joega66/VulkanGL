#include "SceneCommon.glsl"
#include "MeshCommon.glsl"

void main()
{
	vec4 WorldPosition = GetWorldPosition();

	SetVSInterpolants(WorldPosition);

	gl_Position = View.WorldToClip * WorldPosition;
}
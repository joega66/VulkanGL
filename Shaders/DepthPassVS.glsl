#include "SceneCommon.glsl"
#include "MeshCommon.glsl"

void main()
{
	vec4 WorldPosition = GetWorldPosition();
	gl_Position = View.WorldToClip * WorldPosition;
}
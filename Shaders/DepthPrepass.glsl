#include "SceneCommon.glsl"
#include "MeshCommon.glsl"

#ifdef VERTEX_SHADER

void main()
{
	vec4 WorldPosition = GetWorldPosition();

	SetVSInterpolants(WorldPosition);

	gl_Position = View.WorldToClip * WorldPosition;
}

#endif

#ifdef FRAGMENT_SHADER

void main()
{
	DiscardMaskedPixel();
}

#endif
#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "MaterialCommon.glsl"

#ifdef VERTEX_SHADER

void main()
{
	vec4 WorldPosition = Surface_GetWorldPosition();

	Surface_SetAttributes(WorldPosition);

	gl_Position = View.WorldToClip * WorldPosition;
}

#endif

#ifdef FRAGMENT_SHADER

void main()
{
	SurfaceData Surface = Surface_Get();

	Material_DiscardMaskedPixel(Surface);
}

#endif
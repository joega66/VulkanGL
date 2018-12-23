#ifdef STATIC_MESH
#include "StaticMeshVS.glsl"
#endif

uniform ViewUniform
{
	mat4 View;
	mat4 Projection;
} View;

void main()
{
	vec4 WorldPosition = GetWorldPosition();

	SetVSInterpolants(WorldPosition);

	gl_Position = View.Projection * View.View * WorldPosition;
}
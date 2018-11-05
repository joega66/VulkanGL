#ifdef STATIC_MESH
#include "StaticMeshVS.glsl"
#endif

layout(binding = 0) uniform ViewUniform
{
	mat4 View;
	mat4 Projection;
} View;

void main()
{
	SetVSInterpolants();

	vec4 WorldPosition = GetWorldPosition();

	gl_Position = View.Projection * View.View * WorldPosition;
}
#ifdef STATIC_MESH
#include "StaticMeshFS.glsl"
#endif

layout(location = 0) out vec4 OutColor;

void main()
{
	vec4 Diffuse = GetDiffuse();

	OutColor = vec4(Diffuse);
}
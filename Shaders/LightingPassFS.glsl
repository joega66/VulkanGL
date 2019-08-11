#ifdef STATIC_MESH
#include "StaticMeshFS.glsl"
#endif
#include "View.glsl"
#include "LightingCommon.glsl"

layout(location = 0) out vec4 OutColor;

void main()
{
	MaterialParams Material = GetMaterial();

	OutColor = Shade(View.Position, Material);
}
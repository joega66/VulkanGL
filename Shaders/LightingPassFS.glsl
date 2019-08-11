#include "SceneCommon.glsl"
#include "MaterialCommon.glsl"
#include "LightingCommon.glsl"
#ifdef STATIC_MESH
#include "StaticMeshFS.glsl"
#endif

layout(location = 0) out vec4 OutColor;

void main()
{
	MaterialParams Material = GetMaterial();

	OutColor = Shade(View.Position, Material);
}
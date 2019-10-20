#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "LightingCommon.glsl"

layout(location = 0) out vec4 OutColor;

void main()
{
	MaterialParams Material = GetMaterial();

	OutColor = Shade(Material);
}
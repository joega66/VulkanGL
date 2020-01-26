#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "MaterialCommon.glsl"
#include "LightingCommon.glsl"

layout(location = 0) out vec4 OutColor;

void main()
{
	SurfaceData Surface = Surface_Get();

	MaterialData Material = Material_Get(Surface);

	OutColor = Shade(Surface, Material);
}
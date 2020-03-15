#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "MaterialCommon.glsl"
#define SCENE_TEXTURES_SET 3
#define VOXEL_SET 4
#define TRACE_SHADOW_CONE
#include "LightingCommon.glsl"

layout(location = 0) out vec4 OutColor;

void main()
{
	SurfaceData Surface = Surface_Get();

	MaterialData Material = Material_Get(Surface);

	OutColor = Shade(Surface, Material);

	OutColor.xyz = ApplyGammaCorrection(OutColor.xyz);
}
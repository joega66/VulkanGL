#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "MaterialCommon.glsl"
#define SCENE_TEXTURES_SET 3
#define VOXEL_SET 4
#define TRACE_SHADOW_CONE
#include "LightingCommon.glsl"

layout(location = 0) out vec4 OutColor;

vec3 ApplyGammaCorrection(in vec3 Color)
{
	Color = Color / (Color + vec3(1.0));
	Color = pow(Color, vec3(1.0 / 2.2));
	return Color;
}

void main()
{
	SurfaceData Surface = Surface_Get();

	MaterialData Material = Material_Get(Surface);

	OutColor = Shade(Surface, Material);

	OutColor.xyz = ApplyGammaCorrection(OutColor.xyz);
}
#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "MaterialCommon.glsl"
#define CAMERA_TEXTURES_SET 3
#define VOXEL_SET 4
#define TRACE_SHADOW_CONE
#define TRACE_DIFFUSE_CONES
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
	const vec3 V = normalize(Camera.Position - Surface.WorldPosition);

	Material_NormalMapping(Surface, V);

	OutColor = Shade(V, Surface, Material);

	Material_Emissive(Surface, OutColor.xyz);

	OutColor.xyz = ApplyGammaCorrection(OutColor.xyz);
}
#include "CameraCommon.glsl"
#include "MeshCommon.glsl"
#define TEXTURE_SET 2
#define SAMPLER_SET 3
#include "MaterialCommon.glsl"

layout(location = 0) out vec4 OutGBuffer0;
layout(location = 1) out vec4 OutGBuffer1;

void main()
{
	SurfaceData Surface = Surface_Get();
	MaterialData Material = Material_Get(Surface);

	Material_DiscardMaskedPixel(Surface);

	const vec3 V = normalize(Camera.Position - Surface.WorldPosition);

	Material_NormalMapping(Surface, V);

	OutGBuffer0.rgb = Surface.WorldNormal;
	OutGBuffer0.a = Material.Metallic;
	OutGBuffer1.rgb = Material.BaseColor;
	OutGBuffer1.a = Material.Roughness;
}
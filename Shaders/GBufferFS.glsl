#define CAMERA_SET 0
#include "CameraCommon.glsl"
#define MESH_SET 1
#include "MeshCommon.glsl"
#define TEXTURE_SET 2
#define SAMPLER_SET 3
#include "MaterialCommon.glsl"

layout(location = 0) out vec4 outGBuffer0;
layout(location = 1) out vec4 outGBuffer1;

void main()
{
	SurfaceData surface = Surface_Get();
	MaterialData material = Material_Get(surface);

	Material_DiscardMaskedPixel(surface);

	const vec3 v = normalize(_Camera.position - surface.worldPosition);

	Material_NormalMapping(surface, v);

	outGBuffer0.rgb = surface.worldNormal;
	outGBuffer0.a = material.metallic;
	outGBuffer1.rgb = material.baseColor;
	outGBuffer1.a = material.roughness;
}
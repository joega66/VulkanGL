#include "MeshCommon.glsl"
#define TEXTURE_SET 2
#define SAMPLER_SET 3
#include "MaterialCommon.glsl"

void main()
{
	SurfaceData surface = Surface_Get();

	Material_DiscardMaskedPixel(surface);
}
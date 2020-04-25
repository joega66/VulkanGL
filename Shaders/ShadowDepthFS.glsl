#include "MeshCommon.glsl"
#define TEXTURES_SET 2
#define SAMPLERS_SET 3
#include "MaterialCommon.glsl"

void main()
{
	SurfaceData Surface = Surface_Get();

	Material_DiscardMaskedPixel(Surface);
}
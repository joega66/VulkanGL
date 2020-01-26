#include "MeshCommon.glsl"
#include "MaterialCommon.glsl"

void main()
{
	SurfaceData Surface = Surface_Get();

	Material_DiscardMaskedPixel(Surface);
}
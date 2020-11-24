#define MESH_SET 1
#include "MeshCommon.glsl"
#define TEXTURE_SET 2
#include "MaterialCommon.glsl"

void main()
{
	SurfaceData surface = Surface_Get();

	Material_DiscardMaskedPixel(surface);
}
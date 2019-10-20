#include "SceneCommon.glsl"
#include "VoxelsCommon.glsl"
#ifdef STATIC_MESH
#include "StaticMeshFS.glsl"
#endif

layout(location = 4) flat in int InAxis;

void main()
{
	MaterialParams Material = GetMaterial();

	const ivec3 Temp = ivec3(gl_FragCoord.x, gl_FragCoord.y, VOXEL_GRID_SIZE * gl_FragCoord.z);

	ivec3 TexCoord;

	if (InAxis == 1)
	{
		TexCoord.x = VOXEL_GRID_SIZE - Temp.z;
		TexCoord.z = Temp.x;
		TexCoord.y = Temp.y;
	}
	else if (InAxis == 2)
	{
		TexCoord.z = Temp.y;
		TexCoord.y = VOXEL_GRID_SIZE - Temp.z;
		TexCoord.x = Temp.x;
	}
	else
	{
		TexCoord = Temp;
	}

	imageStore(VoxelImage3d, TexCoord, vec4(Material.Albedo, 1.0));
}
#include "Common.glsl"
#include "CameraCommon.glsl"
#include "MeshCommon.glsl"
#define TEXTURE_SET 2
#define SAMPLER_SET 3
#include "MaterialCommon.glsl"
#define VOXEL_SET 4
#include "VoxelsCommon.glsl"

layout(binding = 1, set = VOXEL_SET, rgba8) uniform writeonly image3D VoxelBaseColor;

layout(binding = 2, set = VOXEL_SET, rgba16f) uniform writeonly image3D VoxelNormal;

void main()
{
	vec2 NDCPosition = vec2(gl_FragCoord.xy / vec2(VOXEL_GRID_SIZE)) * 2.0f - 1.0f;

	SurfaceData Surface = Surface_Get();

	Material_DiscardMaskedPixel(Surface);

	MaterialData Material = Material_Get(Surface);

	ivec3 VoxelGridCoord = TransformWorldToVoxelGridCoord(Surface.WorldPosition);

	imageStore(VoxelBaseColor, VoxelGridCoord, vec4(Material.BaseColor, 1.0f));
	imageStore(VoxelNormal, VoxelGridCoord, vec4(Surface.WorldNormal, 1.0f));

#if DEBUG_VOXELS
	uint VoxelIndex = atomicAdd(VoxelDrawIndirect.VertexCount, 1);
	VoxelPositions[VoxelIndex] = EncodeVoxelPosition(ivec3(VoxelGridCoord));
#endif
}
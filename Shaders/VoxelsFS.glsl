#include "Common.glsl"
#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "LightingCommon.glsl"
#include "VoxelsCommon.glsl"

void main()
{
	MaterialParams Material = GetMaterial();

	//vec4 Color = Shade(Material);

	uint VoxelIndex = atomicAdd(VoxelDrawIndirect.VertexCount, 1);
	VoxelColors[VoxelIndex] = ColorToInt(vec4(Material.Albedo, 1.0));
	VoxelPositions[VoxelIndex] = Material.Position;
}
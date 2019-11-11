#include "Common.glsl"
#include "SceneCommon.glsl"
#include "MeshCommon.glsl"
#include "LightingCommon.glsl"
#include "VoxelsCommon.glsl"

void main()
{
	MaterialParams Material = GetMaterial();

	//vec4 Color = Shade(Material);
	vec4 Color = vec4(Material.Albedo, 1);

	uint VoxelIndex = atomicAdd(VoxelDrawIndirect.VertexCount, 1);
	VoxelColors[VoxelIndex] = ColorToInt(Color);
	VoxelPositions[VoxelIndex] = Material.Position;
}
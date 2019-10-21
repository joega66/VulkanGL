layout(location = 0) in vec2 InUV;
layout(location = 0) out vec4 OutColor;

#if VOXELS
layout(binding = 0, set = 0, rgba8) readonly uniform image3D VoxelImage3d;

void VisualizeVoxels()
{
	ivec3 Size = imageSize(VoxelImage3d);
	ivec2 XY = ivec2(vec2(Size.xy) * InUV);

	for (int Z = 0; Z < Size.z; Z++)
	{
		vec4 Color = imageLoad(VoxelImage3d, ivec3(XY, Z));
		if (Color.a > 0.0)
		{
			OutColor = Color;
			return;
		}
	}

	discard;
}
#endif

void main()
{
#if VOXELS
	VisualizeVoxels();
#endif
}
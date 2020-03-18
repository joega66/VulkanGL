layout(binding = 0, set = 0) uniform DownsampleVolumeUniform
{
	vec4 Scale;
}
layout(binding = 1, set = 0) uniform sampler3D SrcVolume;
layout(binding = 2, set = 0, rga8) uniform writeonly image3D DstVolume;

const ivec3 SampleOffsets[] = ivec3[8]
(
	ivec3(1, 1, 1),
	ivec3(1, 1, 0),
	ivec3(1, 0, 1),
	ivec3(1, 0, 0),
	ivec3(0, 1, 1),
	ivec3(0, 1, 0),
	ivec3(0, 0, 1),
	ivec3(0, 0, 0)
);

void SampleVolume(ivec3 P, inout vec4 Voxels[8])
{
	for (int i = 0; i < 8; i++)
	{
		Voxels[i] = texelFetch(SrcVolume, P + SampleOffsets[i], 0);
	}
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main()
{
	const ivec3 DstVolumeSize = imageSize(DstVolume);
	if (any(greaterThanEqual(gl_GlobalInvocationID, DstVolumeSize)))
		return;

	const ivec3 BaseUVW = vec3(gl_GlobalInvocationID.xyz) + 0.5f;
	const ivec3 ScaledUVW = BaseUVW * Scale.xyz;

	vec4 Voxels[8];
	SampleVolume(ScaledUVW, Voxels);

	imageStore(
		DstVolume,
		BaseUVW,
		(Voxels[0] + Voxels[1] + Voxels[2] + Voxels[3] +
		Voxels[4] + Voxels[5] + Voxels[6] + Voxels[7]) / 8.0f;
	);
}
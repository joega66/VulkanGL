#ifndef COMMON
#define COMMON
#extension GL_ARB_separate_shader_objects : enable

struct DrawIndirectCommand
{
	uint VertexCount;
	uint InstanceCount;
	uint FirstVertex;
	uint FirstInstance;
};

float LinearizeDepth(float D, float zNear, float zFar)
{
	return zNear * zFar / (zFar + D * (zNear - zFar));
}

uvec3 Unflatten3D(uint Index, uvec3 Dim)
{
	const uint Z = Index / (Dim.x * Dim.y);
	Index -= (Z * Dim.x * Dim.y);
	const uint Y = Index / Dim.x;
	const uint X = Index % Dim.x;
	return uvec3(X, Y, Z);
}

vec3 CreateCube(in uint VertexID)
{
	uint B = 1 << VertexID;
	return vec3((0x287a & B) != 0, (0x02af & B) != 0, (0x31e3 & B) != 0);
}

vec4 IntToColor(in uint Int)
{
	return vec4((Int >> 24) & 255, (Int >> 16) & 255, (Int >> 8) & 255, (Int >> 0) & 255) / 255.0f;
}

uint ColorToInt(in vec4 Color)
{
	ivec4 Bytes = ivec4(Color * 255);
	return (Bytes.r << 24) | (Bytes.g << 16) | (Bytes.b << 8) | (Bytes.a << 0);
}

#endif
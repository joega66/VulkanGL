#ifndef COMMON
#define COMMON
#extension GL_ARB_separate_shader_objects : enable

 // zFar * zNear / (zFar + depth * (zNear - zFar))
 // clipData[0] = zFar * zNear
 // clipData[1] = zNear - zFar
 // clipData[2] = zFar
float LinearizeDepth(float depth, vec3 clipData)
{
	return clipData[0] / ( depth * clipData[1] + clipData[2] );
}

uvec3 Unflatten3D(uint index, uvec3 dim)
{
	const uint z = index / (dim.x * dim.y);
	index -= (z * dim.x * dim.y);
	const uint y = index / dim.x;
	const uint x = index % dim.x;
	return uvec3(x, y, z);
}

vec3 CreateCube(uint vertexIndex)
{
	uint b = 1 << vertexIndex;
	return vec3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0);
}

vec4 IntToColor(uint intColor)
{
	return vec4(
		float((intColor & 0x000000FF)),
		float((intColor & 0x0000FF00) >> 8U),
		float((intColor & 0x00FF0000) >> 16U),
		float((intColor & 0xFF000000) >> 24U)) / 255.0;
}

uint ColorToInt(vec4 color)
{
	uvec4 bytes = uvec4(color * 255.0);
	return (uint(bytes.w) & 0x000000FF) << 24U
		| (uint(bytes.z) & 0x000000FF) << 16U
		| (uint(bytes.y) & 0x000000FF) << 8U
		| (uint(bytes.x) & 0x000000FF);
}

vec2 CalcLineNormal(vec2 line)
{
	// Rotate the line 90 degrees.
	return normalize(vec2(-line.y, line.x));
}

/** Distance squared. */
float Distance2(vec2 a, vec2 b)
{
	a -= b;
	return dot(a, a);
}

/** Swap a and b. */
void Swap(inout float a, inout float b)
{
	float temp = a;
	a = b;
	b = temp;
}

#endif
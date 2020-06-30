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

uvec3 Unflatten3D(uint Index, uvec3 Dim)
{
	const uint Z = Index / (Dim.x * Dim.y);
	Index -= (Z * Dim.x * Dim.y);
	const uint Y = Index / Dim.x;
	const uint X = Index % Dim.x;
	return uvec3(X, Y, Z);
}

vec3 CreateCube(uint VertexID)
{
	uint B = 1 << VertexID;
	return vec3((0x287a & B) != 0, (0x02af & B) != 0, (0x31e3 & B) != 0);
}

vec4 IntToColor(uint Int)
{
	return vec4(
		float((Int & 0x000000FF)), 
		float((Int & 0x0000FF00) >> 8U),
		float((Int & 0x00FF0000) >> 16U),
		float((Int & 0xFF000000) >> 24U)) / 255.0;
}

uint ColorToInt(vec4 Color)
{
	uvec4 Bytes = uvec4(Color * 255.0);
	return (uint(Bytes.w) & 0x000000FF) << 24U 
		| (uint(Bytes.z) & 0x000000FF) << 16U 
		| (uint(Bytes.y) & 0x000000FF) << 8U 
		| (uint(Bytes.x) & 0x000000FF);
}

vec2 CalcLineNormal(vec2 Line)
{
	// Rotate the line 90 degrees.
	return normalize(vec2(-Line.y, Line.x));
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
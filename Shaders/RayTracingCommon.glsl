#ifndef RAYTRACING_COMMON_H
#define RAYTRACING_COMMON_H

struct ONB
{
	vec3 axis[3];
};

void ONB_BuildFromW(inout ONB onb, vec3 n)
{
	onb.axis[2] = normalize(n);
	const vec3 a = (abs(onb.axis[2].x) > 0.9) ? vec3(0, 1, 0) : vec3(1, 0, 0);
	onb.axis[1] = normalize(cross(onb.axis[2], a));
	onb.axis[0] = cross(onb.axis[2], onb.axis[1]);
}

vec3 ONB_Transform(inout ONB onb, vec3 p)
{
	return p.x * onb.axis[0] + p.y * onb.axis[1] + p.z * onb.axis[2];
}

float length2(vec3 x)
{
	return dot(x, x);
}

uint WangHash(inout uint seed)
{
	seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> 4);
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> 15);
	return seed;
}

/** Generate a random float [0...1] */
float RandomFloat(inout uint seed)
{
	return float(WangHash(seed)) / 4294967296.0;
}

vec3 RandomCosineDirection(inout uint seed)
{
	const float r1 = RandomFloat(seed);
	const float r2 = RandomFloat(seed);
	const float z = sqrt(1 - r2);

	const float phi = 2 * PI * r1;
	const float x = cos(phi) * sqrt(r2);
	const float y = sin(phi) * sqrt(r2);

	return vec3(x, y, z);
}

#endif
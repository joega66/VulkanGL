#ifndef RAYTRACING_COMMON_H
#define RAYTRACING_COMMON_H

// Reference: https://www.shadertoy.com/view/wts3RX
float CubeRoot(float x)
{
	const int NEWTON_ITER = 2;
	const int HALLEY_ITER = 0;

	float y = sign(x) * uintBitsToFloat(floatBitsToUint(abs(x)) / 3u + 0x2a514067u);

	for (int i = 0; i < NEWTON_ITER; ++i)
		y = (2. * y + x / (y * y)) * .333333333;

	for (int i = 0; i < HALLEY_ITER; ++i)
	{
		float y3 = y * y * y;
		y *= (y3 + 2. * x) / (2. * y3 + x);
	}

	return y;
}

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

/** Length squared. */
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

float RandomFloat(inout uint seed, float min, float max)
{
	return min + (max - min) * RandomFloat(seed);
}

vec3 RandomVec3(inout uint seed, float min, float max)
{
	return vec3(RandomFloat(seed, min, max), RandomFloat(seed, min, max), RandomFloat(seed, min, max));
}

// Reference: https://karthikkaranth.me/blog/generating-random-points-in-a-sphere/
vec3 RandomInUnitSphere(inout uint seed)
{
	float x1 = RandomFloat(seed);
	float x2 = RandomFloat(seed);
	float x3 = RandomFloat(seed);

	const float mag = sqrt(x1 * x1 + x2 * x2 + x3 * x3);

	x1 /= mag;
	x2 /= mag; 
	x3 /= mag;

	const float c = CubeRoot(RandomFloat(seed));

	return vec3(x1 * c, x2 * c, x3 * c);
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
#ifndef RAYTRACING_COMMON_H
#define RAYTRACING_COMMON_H

// Reference: https://www.shadertoy.com/view/wts3RX
float CubeRoot(float x)
{
	const int NEWTON_ITER = 2;
	const int HALLEY_ITER = 0;

	float y = sign(x) * uintBitsToFloat(floatBitsToUint(abs(x)) / 3u + 0x2a514067u);

	for (int i = 0; i < NEWTON_ITER; ++i)
	{
		y = (2. * y + x / (y * y)) * .333333333;
	}

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
	return normalize( p.x * onb.axis[0] + p.y * onb.axis[1] + p.z * onb.axis[2] );
}

/** Length squared. */
float length2(vec3 x)
{
	return dot(x, x);
}

uint RandomInit(ivec2 screenCoords, uint frameNumber)
{
	return uint(uint(screenCoords.x) * uint(1973) + uint(screenCoords.y) * uint(9277) + frameNumber * uint(26699)) | uint(1);
}

uint RandomNext(inout uint seed)
{
	seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> 4);
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> 15);
	return seed;
}

/** Generate a random float [0...1]. */
float RandomFloat(inout uint seed)
{
	return float(RandomNext(seed)) / 4294967296.0;
}

uint seed;
/** Generate a random float [0...1] using the global seed. */
float RandomFloat()
{
	return float(RandomNext(seed)) / 4294967296.0;
}

float RandomFloat(float min, float max)
{
	return min + (max - min) * RandomFloat();
}

vec3 RandomVec3(float min, float max)
{
	return vec3(RandomFloat(min, max), RandomFloat(min, max), RandomFloat(min, max));
}

// Reference: https://karthikkaranth.me/blog/generating-random-points-in-a-sphere/
vec3 RandomInUnitSphere()
{
	float x1 = RandomFloat();
	float x2 = RandomFloat();
	float x3 = RandomFloat();

	const float mag = sqrt(x1 * x1 + x2 * x2 + x3 * x3);

	x1 /= mag;
	x2 /= mag; 
	x3 /= mag;

	const float c = CubeRoot(RandomFloat());

	return vec3(x1 * c, x2 * c, x3 * c);
}

vec3 RandomCosineDirection()
{
	const float r1 = RandomFloat();
	const float r2 = RandomFloat();
	const float z = sqrt(1 - r2);

	const float phi = 2 * PI * r1;
	const float x = cos(phi) * sqrt(r2);
	const float y = sin(phi) * sqrt(r2);

	return vec3(x, y, z);
}

float VanDerCorpus(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint sampleIndex, const uint numSamples)
{
	return vec2(VanDerCorpus(sampleIndex), float(sampleIndex) / float(numSamples));
}

// Reference: Ray Tracing Gems, Section 16.6.5, GGX DISTRIBUTION
vec3 GGX_ImportanceSample(float alpha, inout float cosH)
{
	const vec2 u = vec2(RandomFloat(), RandomFloat());
	cosH = sqrt((1 - u[0]) / ((alpha * alpha - 1) * u[0] + 1));
	const float sinH = sqrt(1 - cosH * cosH);
	const float phi = 2 * PI * u[1];

	const float x = cos(phi) * sinH;
	const float y = sin(phi) * sinH;
	const float z = cosH;

	return normalize(vec3(x, y, z));
}

float GGX_ScatteringPDF(float ndf, float vdoth, float cosH)
{
	return ndf * cosH / (4 * vdoth);
}

float Schlick(float cosTheta, float refractiveIndex)
{
	float r0 = (1 - refractiveIndex) / (1 + refractiveIndex);
	r0 = r0 * r0;
	return r0 + (1 - r0) * pow((1 - cosTheta), 5);
}

float CosinePDF(vec3 normal, vec3 direction)
{
	const float cosine = dot(normal, direction);
	return cosine <= 0 ? 0 : cosine / PI;
}

float Lambertian_ScatteringPDF(vec3 normal, vec3 direction)
{
	return CosinePDF(normal, direction);
}

#endif
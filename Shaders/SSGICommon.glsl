#ifndef SSGI_COMMON_H
#define SSGI_COMMON_H
#include "Common.glsl"

/** Reference: "Efficient GPU Screen-Space Ray Tracing" (2014) */
bool TraceScreenSpaceRay(
	vec3			csOrigin,
	vec3			csDirection,
	mat4			viewToPixel,
	sampler2D       zBuffer,
	vec2			zBufferSize,
	vec3			clipData,
	float           zThickness,
	float			stride,
	float           jitterFraction,
	float           maxSteps,
	float			maxRayTraceDistance,
	out vec2		hitPixel,
	out vec3		csHitPoint)
{
	vec3 csEndPoint = csOrigin + csDirection * maxRayTraceDistance;
	
	vec4 h0 = viewToPixel * vec4(csOrigin, 1.0);
	vec4 h1 = viewToPixel * vec4(csEndPoint, 1.0);

	float k0 = 1.0 / h0.w;
	float k1 = 1.0 / h1.w;

	vec3 q0 = csOrigin * k0;
	vec3 q1 = csEndPoint * k1;

	vec2 p0 = h0.xy * k0;
	vec2 p1 = h1.xy * k1;

	// @todo-joe
	p0.xy = (p0.xy + 1.0) / 2.0;
	p1.xy = (p1.xy + 1.0) / 2.0;
	p0.xy *= zBufferSize;
	p1.xy *= zBufferSize;

	hitPixel = vec2(-1.0, -1.0);

	p1 += vec2((Distance2(p0, p1) < 0.0001) ? 0.01 : 0.0);

	vec2 delta = p1 - p0;

	bool permute = false;
	if (abs(delta.x) < abs(delta.y))
	{
		permute = true;

		delta = delta.yx;
		p1 = p1.yx;
		p0 = p0.yx;
	}

	float stepDirection = sign(delta.x);
	float invdx = stepDirection / delta.x;
	vec2 dP = vec2(stepDirection, invdx * delta.y);

	vec3 dQ = (q1 - q0) * invdx;
	float dk = (k1 - k0) * invdx;

	dP *= stride; dQ *= stride; dk *= stride;

	p0 += dP * jitterFraction; q0 += dQ * jitterFraction; k0 += dk * jitterFraction;

	vec3 q = q0;
	float k = k0;

	float prevZMaxEstimate = csOrigin.z;
	float stepCount = 0.0;
	float rayZMax = prevZMaxEstimate, rayZMin = prevZMaxEstimate;
	float sceneZMax = rayZMax + 1e4;

	float end = p1.x * stepDirection;

	for (vec2 p = p0;
		((p.x * stepDirection) <= end) &&
		(stepCount < maxSteps) &&
		((rayZMax < sceneZMax - zThickness) || (rayZMin > sceneZMax)) &&
		(sceneZMax != 0.0);
		p += dP, q.z += dQ.z, k += dk, stepCount += 1.0)
	{
		hitPixel = permute ? p.yx : p;

		rayZMin = prevZMaxEstimate;

		rayZMax = (dQ.z * 0.5 + q.z) / (dk * 0.5 + k);
		prevZMaxEstimate = rayZMax;

		if (rayZMin > rayZMax) { Swap(rayZMin, rayZMax); }

		sceneZMax = texelFetch(zBuffer, ivec2(hitPixel), 0).r;
		sceneZMax = LinearizeDepth(sceneZMax, clipData);
	}

	q.xy += dQ.xy * stepCount;
	csHitPoint = q * (1.0 / k);

	return (rayZMax >= sceneZMax - zThickness) && (rayZMin <= sceneZMax);
}

#endif